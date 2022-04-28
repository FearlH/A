#include "net/EpollPoller.h"
#include "base/Logging.h"
#include "net/Channel.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
using namespace m2;
using namespace net;

namespace
{
    const int KNew = -1;    //表示是新的，没有加入Epoll而且不在fd_channels里面
    const int KAdded = 1;   //表示已经加入Epoll而且在fd_channels里面
    const int KDeleted = 2; //表示的是已经删除的，不在epoll里面，但是在fd_channels里面
}

EpollPoller::EpollPoller(EventLoop *loop) : Poller(loop), epollFd_(::epoll_create1(EPOLL_CLOEXEC)), resultEvents_(KInitEventListSize_)
{
    if (epollFd_ < 0)
    {
        LOG_SYSFATAL << "EPOLL EVENT ERROR";
    }
}
EpollPoller::~EpollPoller()
{
    ::close(epollFd_);
}

//对于这个Channel，去更新Poller里面的Epoll
// Channel里面已经有了 event了，所以还需要知道Channel在Poller里面的状态，就需要Channel的index_
//添加一个non的event就是去从epoll监视器里面删除事件
void EpollPoller::updateChannel(Channel *channel)
{
    assetInLoopThread();
    int fd = channel->fd();
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd()
              << " events = " << channel->events() << " index = " << index;
    if (index == KNew || index == KDeleted)
    {
        if (index == KNew)
        {
            assert(fd_Channels_.find(fd) == fd_Channels_.end());
            fd_Channels_[fd] = channel;
        }
        else if (index == KDeleted)
        {
            assert(fd_Channels_.find(fd) != fd_Channels_.end());
            assert(fd_Channels_[fd] == channel);
        }
        channel->setIndex(KAdded);
        updateInEpoll(EPOLL_CTL_ADD, channel);
    }
    else
    {
        assert(fd_Channels_.find(fd) != fd_Channels_.end());
        assert(fd_Channels_[fd] == channel);
        assert(channel->index() == KAdded);
        if (channel->isNoneEvent())
        {
            updateInEpoll(EPOLL_CTL_DEL, channel);
        }
        else
        {
            updateInEpoll(EPOLL_CTL_MOD, channel);
        }
    }
}

//彻底删除这个事件
void EpollPoller::removeChannel(Channel *channel)
{
    assetInLoopThread();
    //彻底删除
    const int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    assert(fd_Channels_.find(fd) != fd_Channels_.end());
    assert(fd_Channels_[fd] == channel);
    assert(channel->isNoneEvent());
    const int index = channel->index();
    assert(index == KDeleted || index == KAdded);
    fd_Channels_.erase(fd);
    if (index == KAdded)
    {
        updateInEpoll(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(KNew); //彻底删除
}

void EpollPoller::fillActiveChannleList(int numEvents,
                                        ChannelList *list) const
{
    // assert(numEvents <= resultEvents_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *thisEventChannel = static_cast<Channel *>(resultEvents_[i].data.ptr); //还需要设置事件
        // Epoll里面放的是ptr
        thisEventChannel->set_revents(resultEvents_[i].events);
        list->push_back(thisEventChannel); //现在的epoll里面存的就是Channel的指针
    }
}

// EventLoop会调用这个Pooler的poll函数,给一个时间，然后把这个Poller里面的就绪的Channel返回
Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    int eventNums = ::epoll_wait(epollFd_, resultEvents_.data(), resultEvents_.size(), timeoutMs);
    int savedErrno = errno;
    Timestamp timeNow = Timestamp::now();
    if (eventNums > 0)
    {
        fillActiveChannleList(eventNums, activeChannels);
        if (eventNums == resultEvents_.size())
        {
            resultEvents_.resize(resultEvents_.size() * 2);
        }
    }
    else if (eventNums == 0)
    {
        LOG_TRACE << "NO EVENT NOTHING HAPPENDS";
    }
    else
    {
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "ERROR EPOLL EpollPoller::poll()";
        }
    }
    return timeNow;
}

void EpollPoller::updateInEpoll(int options, Channel *channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    int fd = channel->fd();
    event.events = channel->events();
    event.data.ptr = channel;
    LOG_TRACE << "Epoll ->EpollPoller::update() fd: " << fd;
    if (::epoll_ctl(epollFd_, options, fd, &event) < 0)
    {
        if (options == EPOLL_CTL_DEL)
        {
            LOG_SYSERR << "Epoll error ->fd: " << fd << operationToString(options);
        }
        else
        {
            LOG_SYSFATAL << "Epoll error ->fd: " << fd << operationToString(options);
        }
    }
}

const char *EpollPoller::operationToString(int op)
{
    switch (op)
    {
    case EPOLL_CTL_ADD:
        return "ADD";
    case EPOLL_CTL_DEL:
        return "DEL";
    case EPOLL_CTL_MOD:
        return "MOD";
    default:
        assert(false && "ERROR op");
        return "Unknown Operation";
    }
}
