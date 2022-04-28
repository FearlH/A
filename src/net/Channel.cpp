#include "net/Channel.h"

#include <assert.h>
#include <sstream>
#include <sys/types.h>
#include <sys/epoll.h>

using namespace m2;
using namespace net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd),
      events_(0), revents_(0),
      index_(-1), //新加入的就是没有index
      logHup_(true),
      tie_(),
      tied_(false),
      eventHandling_(false),
      addedToLoop_(false)
{
}

Channel::~Channel()
{
    // 析构的时候怎么确定loop是否存在？？
    // 都是单线程操作，自己定义好顺序就行了
    // assert(!addedToLoop_);
    // assert(!eventHandling_);
    //没有工作去作
    if (loop_->isInLoopThread())
    {
        // assert(!loop_->hasChannel(this)); //确定loop里面不含有这个了
    }
}

void Channel::tie(const std::shared_ptr<void> &toTie)
{
    tie_ = toTie; //使用的是weak_ptr
    //这样也绑定不上呀
    tied_ = true;
}

void Channel::remove()
{
    assert(!isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard) //不控制变量的生存期，所以需要判断是不是有提升
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    eventHandling_ = true;
    LOG_TRACE << eventsToString();
    // revents_就是返回到的event
    if (revents_ & EPOLLHUP && !events_ & EPOLLIN) //对面发出了shutdowm(fd,WR)event时间就会有POLLHUP
    {
        if (logHup_)
        {
            LOG_WARN << "fd= " << fd_ << "Channel::handleEvent() HUP";
        }
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // if (revents_ & EPOLLNVAL)
    // {
    //     //不存在的文件描述符
    //     LOG_WARN << "fd= " << fd_ << "Channel::handleEvent() INVALID";
    // }
    if (revents_ & EPOLLERR)
    {
        LOG_WARN << "fd= " << fd_ << "Channel::handleEvent() ERROR";
        //出错使用的是error
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    if (revents_ & (EPOLLIN | EPOLLRDHUP | EPOLLPRI)) // POLLPRI紧急事件
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    if (writeCallback_)
    {
        writeCallback_();
    }
    eventHandling_ = false;
}

void Channel::update()
{
    addedToLoop_ = true;
    ownerLoop()->updateChannel(this);
}

string Channel::eventsToString() const
{
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev) //参数不一样
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & EPOLLIN)
        oss << "IN ";
    if (ev & EPOLLPRI)
        oss << "PRI ";
    if (ev & EPOLLOUT)
        oss << "OUT ";
    if (ev & EPOLLHUP)
        oss << "HUP ";
    if (ev & EPOLLRDHUP)
        oss << "RDHUP ";
    if (ev & EPOLLERR)
        oss << "ERR ";

    return oss.str();
}
