#include "TcpConnection.h"
#include "base/Logging.h"
#include "base/WeakCallback.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketOps.h"

#include <functional>

#include <errno.h>

using std::placeholders::_1;

using namespace m2;
using namespace m2::net;

void m2::net::defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void m2::net::defaultMessageCallback(const TcpConnectionPtr &ptr, Buffer *buffer, Timestamp time)
{
    return buffer->retrieveAll(); //默认是丢弃所有的数据
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(loop),
      name_(nameArg),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr_),
      reading_(true),
      highWaterMark_(64 * 1024 * 1024),
      channel_(new Channel(loop, sockfd))

{
    //!因为是listen的loop创建一个TcpConnection到一个ioLoop的里面
    //!构造函数是在listen线程里面执行的
    //!所以不会去使用有关loop_的一些函数(不在一个线程里面)
    //!会在后面调用runInLoop完成初始化
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallbacks(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallbacks(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallbacks(std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd;
    socket_->setKeepAlive(true); //?感觉这个保活没有什么用处
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
    assert(state_ == kDisconnected);
}
bool TcpConnection::getTcpInfo(struct tcp_info *info) const
{
    return socket_->getTcpInfo(info); //包装的一个系统调用
}

string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

void TcpConnection::send(const void *data, int len)
{
    send(StringPiece(static_cast<const char *>(data), len));
}

void TcpConnection::send(const StringPiece &message) //?使用StringPiece有点类似于代替移动操作
{
    /*
    ?因为这个可能不是在loop线程里面调用的
    ?所以会派送一个任务到loop线程，那么就会产生一些延迟
    ?所以这里不使用atomic也是可以的(因为延迟反正可能会导致现在判断的时候是连接中，之后执行的时候就是断开了)
     */
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp,
                          this,                  // FIXME
                          message.as_string())); //其实是可以直接移动的
            // std::forward<string>(message)));
        }
    }
}

void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp,
                          this, // FIXME
                          buf->retrieveAllAsString()));
            /*
            感觉不能直接移动buffer里面的数据，或者直接操作buffer里面的数据去写
            因为同时大家都在操作buffer，那么除非每个都把buffer全部写完，不然就会导致buffer里面的数据移动了
            而且如果直接调用buffer的移动类的函数，那么就会使得原始的buffer内存失效
            还是需要内存分配
            */
            // std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nWrite = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected) //?转移到这里去判断
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // isWriting表示注册了可写任务，注册了说明是之前有任务要写，那么之前就有数据需要从这个sockfd里面写
    //自己就不能直接写
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nWrite = socket::write(channel_->fd(), data, len);
        if (nWrite >= 0)
        {
            remaining -= nWrite;
            if (remaining == 0)
            {
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    //!使用智能指针，确保任务在执行的时候，这个单位不会被析构
                }
            }
        }
        else
        {
            nWrite = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) //屏蔽致命错误
                {
                    faultError = true;
                }
            }
        }
    }
    assert(remaining <= len);
    if (!faultError && remaining != 0) //有致命的错误就不会写
    {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ &&
            oldLen < highWaterMark_ &&
            highWaterMark_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
            //!使用智能指针，确保任务在执行的时候，这个单位不会被析构
        }
        outputBuffer_.append(static_cast<const char *>(data) + nWrite, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() //用户会调用这个函数，然后调用的时候就应该立即改变状态
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this)); //?这里为什么是this,不是shared_from_this
    }
}

void TcpConnection::shutdownInLoop() //用户不会调用这个函数
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) //正在写不能关闭，要等待全部写完再去关闭
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        loop_->runAfter(seconds,
                        makeWeakCallback(shared_from_this(), &TcpConnection::forceCloseInLoop));
        //?把这个对象用weak_ptr绑定，这个对象的成员函数
        //?delay之前有可能已经关闭了?
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        handleClose();
    }
}

const char *TcpConnection::stateToString() const
{
    switch (state_)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading())
    {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading())
    {
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished() //构造好了之后连接建立，然后就会调用这个函数
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() //!从外面传进来的是shared_from_this,就是保留最后不被析构
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this()); //?断开连接也需要ConnectionCallback
    }
    channel_->remove();
    //在这里正向析构
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno); //数据读到buffer的里面
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0) //读到0就说明是关闭了
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite() //开始的时候是直接写数据，在数据没有写完的时候才会注册handleWrite
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t nWrite = socket::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        if (nWrite > 0)
        {
            outputBuffer_.retrieve(nWrite);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(&TcpConnection::writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop(); //全部写完之后再去关闭
                }
            }
        }
        else //可写了，然后也没有写进去
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
        }
    }
    else
    {
        //出错，报告可写，但是Channle没有注册可写
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);
    TcpConnectionPtr guardThis = shared_from_this();
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
    //!不关闭sockfd，等到析构的时候，会析构socket_,那个里面会关闭sockfd
}

void TcpConnection::handleError()
{
    int err = socket::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}