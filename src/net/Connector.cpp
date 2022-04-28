#include "net/Connector.h"

#include "base/Logging.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/SocketOps.h"

#include <functional>

#include <errno.h>

using namespace m2;
using namespace m2::net;
const int Connector::KMaxRetryDelayMs;
const int Connector::KInitRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop), serverAddress_(serverAddr), connect_(false), state_(kDisconnected), retryDelayMs_(KInitRetryDelayMs)
{
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() //析构的时候都不会做一些清理工作
{
    LOG_DEBUG << "dtor[" << this << "]";
    assert(!channel_);
}
//可能是因为外部有关闭的函数

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}
void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel(); //?
        retry(sockfd);                        //?
    }
}
void Connector::connect()
{
    int sockfd = socket::createNonblockingOrDie(serverAddress_.family());
    int n = socket::connect(sockfd, serverAddress_.getSockAddr());
    int savedErrno = (n >= 0 ? 0 : errno);
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS: //连接还在进行中
    case EINTR:       //中断为什么还是确认连接成功//?非阻塞被中断了还会继续连接?
    case EISCONN:     //已经连接
        connecting(sockfd);
        break;
    case EAGAIN: //没有足够的空闲本地端口
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED: //远程地址没有处于监听状态
    case ENETUNREACH:  // net un reach;网络不可达
        retry(sockfd);
        break;
    case EACCES:       //连接不上，不可广播的 连接广播，或者是防火墙导致的失败
    case EPERM:        //和上面差不多
    case EAFNOSUPPORT: //不支持的地址族
    case EALREADY:     //非阻塞套接子，且之前的连接还没有完成
    case EBADF:        //非法的文件描述符
    case EFAULT:       //出错
    case ENOTSOCK:
        LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
        socket::close(sockfd);
        break;
    default:
        LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
        socket::close(sockfd);
        // connectErrorCallback_();
        break;
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = KInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    //查看是否可读，可读就是连接成功
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallbacks(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallbacks(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() //执行完成之后Channel就变成了空的
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

void Connector::handleWrite() //链接上之后一定可以去写，那么就是使用handleWrite
{
    LOG_TRACE << "Connector::handleWrite " << state_;

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        //连接，每链接上一个那么就不需要再去(使用Connector)关注这个文件描述符了
        //把这个文件描述符交给上面处理
        int err = socket::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror_tl(err);
            retry(sockfd);
        }
        else if (socket::isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                socket::close(sockfd);
            }
        }
    }
    else
    {
        // what happened?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state=" << state_;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
    socket::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddress_.toIpPort()
                 << " in " << retryDelayMs_ << " milliseconds. ";
        loop_->runAfter(retryDelayMs_ / 1000.0,
                        std::bind(&Connector::startInLoop, shared_from_this())); //会重新设置fd
        retryDelayMs_ = std::min(retryDelayMs_ * 2, KMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}