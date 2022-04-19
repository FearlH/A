#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <thread>
#include <string>

using namespace m2;
using namespace m2::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &name)
    : baseLoop_(baseLoop), name_(name), started_(false), numThreads_(0), next_(0), threads_(), loops_()
{
}
EventLoopThreadPool::~EventLoopThreadPool() {}
void EventLoopThreadPool::start(const ThreadInitCallback &callback)
{
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        std::string name = name_;
        name += std::to_string(i);
        EventLoopThread *loopThread = new EventLoopThread(callback, name);
        threads_.push_back(std::unique_ptr<EventLoopThread>(loopThread));
        loops_.push_back(loopThread->strat());
    }
    if (numThreads_ == 0 && callback) //??为什么需要等于0之后再去调用
    {
        callback(baseLoop_);
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    EventLoop *loop = baseLoop_;
    //??如果有pool里面的loop就不会返回baseLoop? why
    if (!loops_.empty()) //??if (numThreads_)
    {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) //向上转型
        {
            next_ = 0;
        }
    }
    return loop;
}

EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashcode)
{
    baseLoop_->assertInLoopThread();
    EventLoop *loop = baseLoop_;
    if (!loops_.empty()) //??if (numThreads_)
    {
        loop = loops_[hashcode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    if (!loops_.empty()) //??if (numThreads_)
    {
        return {baseLoop_};
    }
    return loops_;
}
