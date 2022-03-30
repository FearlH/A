#include "EventLoop.h"
#include "CurrentThread.h"
#include <assert.h>
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
    assertInLoopThread();//保证loop函数只能在创建这个对象的线程里面调用
    LOG_TRACE << "START TIME";
    //...
    LOG_TRACE << "END TIME";
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "NOT in loop thread " << t_loopInThread << " ID: " << threadId_;
}