#include "net/TimerQueue.h"
#include "base/Logging.h"
#include "net/Timer.h"
#include "net/TimerId.h"
#include "net/EventLoop.h"
#include "base/Timestamp.h"
#include <net/Callbacks.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <memory>
#include <assert.h>
#include <string.h>
#include <algorithm>

namespace m2
{
    namespace net
    {
        namespace detail
        {
            int createTimerfd()
            {
                int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                               TFD_NONBLOCK | TFD_CLOEXEC);
                if (timerfd < 0)
                {
                    LOG_SYSFATAL << "Failed in timerfd_create";
                }
                return timerfd;
            }
            struct timespec howMuchTimeFromNow(Timestamp when)
            {
                int64_t microseconds = when.microsecondsFromEpoch() - Timestamp::now().microsecondsFromEpoch();
                if (microseconds < 100)
                {
                    microseconds = 100;
                }
                struct timespec ts;
                ts.tv_sec = static_cast<time_t>(
                    microseconds / Timestamp::KMicrosecondsPerSecond);
                ts.tv_nsec = static_cast<long>(
                    (microseconds % Timestamp::KMicrosecondsPerSecond) * 1000);
                return ts;
            }

            void readTimerfd(int timerfd, Timestamp now)
            {
                uint64_t howmany;
                ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
                LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
                if (n != sizeof howmany)
                {
                    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
                }
            }

            void resetTimerfd(int timerfd, Timestamp expiration)
            {
                // wake up loop by timerfd_settime()
                struct itimerspec newValue;
                struct itimerspec oldValue;
                memset(&newValue, 0, sizeof newValue);
                memset(&oldValue, 0, sizeof oldValue);
                newValue.it_value = howMuchTimeFromNow(expiration);
                int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
                if (ret)
                {
                    LOG_SYSERR << "timerfd_settime()";
                }
            }
        }
    }
}

using namespace m2;
using namespace net;
using namespace net::detail;

TimerQueue::TimerQueue(EventLoop *loop) : loop_(loop), timerfd_(createTimerfd()), timerfdChannel_(loop, timerfd_)
{
    timerfdChannel_.setReadCallback([this](Timestamp time)
                                    { this->hadleRead(); });
    timerfdChannel_.enableReading();
}
TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for (auto &entry : timerList_)
    {
        delete entry.second;
    }
}
bool TimerQueue::insert(Timer *time)
{
    //直接把时间添加到set的里面
    loop_->assertInLoopThread();
    // assert(timerList_.size()==activeTimers_.size());//???
    bool earlistChanged = false;
    auto it = timerList_.begin();
    if (it == timerList_.end() || time->expiration() < it->first)
    {
        earlistChanged = true;
    }
    timerList_.insert({time->expiration(), std::move(time)});
    activeTimers_.insert(ActiveTimer{time, time->sequence()});
    return earlistChanged;
}

void TimerQueue::addTimerInLoop(Timer *timer) //在这个Loop线程里面使用Loop里面的TimerQueue对象了
{
    loop_->assertInLoopThread();
    if (insert(timer))
    {
        resetTimerfd(timerfd_, timer->expiration()); //里面会计算距离现在还有多长的时间过期
        //因为新加入的是最短的过期时间，还需要去设置timerfd_的过期时间
    }
}

TimerId TimerQueue::addTimer(VoidFunc timeCallback, Timestamp when, double interval)
{
    Timer *timer = new Timer(timeCallback, when, interval);
    loop_->runInLoop([this, timer]
                     { this->addTimerInLoop(timer); });
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop([this, timerId]
                     { this->cancelTimerInLoop(timerId); });
}

void TimerQueue::cancelTimerInLoop(TimerId timeId)
{
    loop_->assertInLoopThread();
    assert(timerList_.size() == activeTimers_.size());
    ActiveTimerSet::iterator it = activeTimers_.find({timeId.timer_, timeId.sequence_}); //声明为友元
    if (it != activeTimers_.end())
    {
        timerList_.erase(Entry{it->first->expiration(), it->first}); //存放的是到期时间和指针
        delete it->first;
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_) //?本线程已经执行cancelInLoop了那么怎么会还是在callingExpiredTimers呢？
    {
        cancelTimers_.insert({timeId.timer_, timeId.sequence_});
    }
}

void TimerQueue::hadleRead()
{
    //如果是在HandelRead之前取消的，那么就会随着到期加入到过期事件里面
    //过期事件只要是不重复的，那么就会被删除
    //?可能是因为多线程调用使得cancle比add来的要早
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    //是handleRead的时候的now，那么就是还会有一些过期的时间出线

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelTimers_.clear(); //?为什么在这里需要清空
    // safe to callback outside critical section
    for (Entry &it : expired)
    {
        it.second->run(); //做完所有的定时器事件
    }
    //?这些callback可能会在别的线程里面运行
    callingExpiredTimers_ = false;
    //先把过期的都拿出来，然后执行，最后更新计时器

    reset(expired, now); //过期的事件里面如果有重复的，那么就重新设置新的定时器
    // cancelTimers_.clear(); //?感觉是在这里需要清空
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    // now是handleRead的时候的now，那么就是还会有一些过期的时间出线
    //会把过期的都从set里面拿出来
    std::vector<Entry> expired;
    //获得过期的定时器
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    std::set<Entry>::iterator end = timerList_.lower_bound(sentry);
    assert(end == timerList_.end() || now < end->first);
    std::copy(timerList_.begin(), end, back_inserter(expired));
    timerList_.erase(timerList_.begin(), end);

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        // assert(n == 1);
        // (void)n;
    }

    assert(timerList_.size() == activeTimers_.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat() &&
            cancelTimers_.find(timer) == cancelTimers_.end()) //重复而且没有被注销的
        {
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            // FIXME move to a free list
            delete it.second; // FIXME: no delete please
        }
    }

    //后面就是更新时间定时器的到期时间
    if (!timerList_.empty())
    {
        nextExpire = timerList_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}