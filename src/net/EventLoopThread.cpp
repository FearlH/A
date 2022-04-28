#include "net/EventLoopThread.h"

using namespace m2;
using namespace net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &callback, const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      callback_(callback),
      threadFunc_([this]
                  {
                        EventLoop loop;//threadFunc这个就是在栈的上面，伴随Thread
                        if(this->callback_)
                        {
                           this->callback_(&loop);
                        }
                        {
                            std::unique_lock<std::mutex> ulocker(this->mutex_);
                            this->loop_ = &loop;
                            this->cond_.notify_one();
                        }
                        loop.loop(); }),
      name_(name)

{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_)
    {
        loop_->quit();
        loopThread_.join();
    }
}

EventLoop *EventLoopThread::strat()
{

    std::thread t(threadFunc_);
    {
        std::unique_lock<std::mutex> ulock(mutex_);
        cond_.wait(ulock, [this]
                   { return this->loop_ != nullptr; });
    }
    loopThread_ = std::move(t);
    return loop_;
}