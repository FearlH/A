#include "EventLoop.h"
#include "CurrentThread.h"
#include "Channel.h"
#include "Poller.h"
#include <assert.h>
#include <algorithm>
using namespace m2;
using namespace net;

namespace
{
    thread_local EventLoop *t_loopInThread = nullptr; //这个线程拥有的Loop
}

EventLoop::EventLoop() : looping_(false), threadId_(CurrentThread::tid())
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
    //...
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