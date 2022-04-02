#include "EventLoop.h"
#include "base/CurrentThread.h"
#include "Channel.h"
#include "Poller.h"
#include "base/Timestamp.h"
#include <assert.h>
#include <algorithm>
using namespace m2;
using namespace net;

namespace
{
    thread_local EventLoop *t_loopInThread = nullptr; //这个线程拥有的Loop
    const int kPollTimeMs = 10000;                    // Epoll等待的时间
}

EventLoop::EventLoop() : looping_(false), threadId_(CurrentThread::tid()),
                         poller_(Poller::newDefaultPoller(this)), quit_(false),
                         eventHandling_(false), iteration_(0),
                         currentActiveChannel_(nullptr)

// looping_(false),
// quit_(false),
// eventHandling_(false),
// callingPendingFunctors_(false),
// iteration_(0),
// threadId_(CurrentThread::tid()),
// poller_(Poller::newDefaultPoller(this)),
// timerQueue_(new TimerQueue(this)),
// wakeupFd_(createEventfd()),
// wakeupChannel_(new Channel(this, wakeupFd_)),
// currentActiveChannel_(NULL)
{
    LOG_TRACE << "Thread Loop Created ID: " << threadId_;
    if (t_loopInThread != nullptr)
    {
        LOG_FATAL << "Already exist a Loop " << t_loopInThread << " in " << threadId_;
    }
    else
    {
        t_loopInThread = this;
    }
}

EventLoop::~EventLoop()
{
    looping_ = false;
    t_loopInThread = nullptr;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread(); //保证loop函数只能在创建这个对象的线程里面调用
    LOG_TRACE << "START TIME";
    looping_ = true;
    quit_ = false;
    //...

    // ChannelList activeChannels_;
    // Channel *currentActiveChannel_;

    // while (1)
    // {

    //     activeChannels_.resize(0);
    //     poller_->poll(10, &activeChannels_);
    //     for (auto c : activeChannels_)
    //     {
    //         c->handleEvent(Timestamp::now());
    //     }
    // }

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_; //不知道是干什么
        eventHandling_ = true;
        for (auto channel : activeChannels_)
        {
            currentActiveChannel_ = channel;
            channel->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        // doPendingFunctors();
    }
    LOG_TRACE << "END TIME";
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "NOT in loop thread " << t_loopInThread << " ID: " << threadId_;
}

void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    //当前处理的这一个就是，或者是没有
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this); // channel必须是自己的
    assertInLoopThread();                 //这个就是必须的，需要在当前线程执行
    return poller_->hasChannel(channel);
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        // wakeup();
        //异步唤醒，在另外一个线程里面退出的时候，有可能这个loop正在等待在Epoll上面
        //那么就会需要去唤醒
    }
}