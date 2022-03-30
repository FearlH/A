#include "CountDownLatch.h"
using namespace m2;

void CountDownLatch::wait()
{
    std::unique_lock<std::mutex> locker(mtx_);
    cond_.wait(locker, [&]
               { return count_ == 0; }); //里面的是醒来的条件
    //函数返回的时候就不在锁的里面了
}

void CountDownLatch::countDown()
{
    std::lock_guard<std::mutex> locker(mtx_);
    --count_;
    if (count_ == 0)
    {
        cond_.notify_all();
    }
}