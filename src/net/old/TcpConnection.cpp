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