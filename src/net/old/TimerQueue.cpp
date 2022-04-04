#include "TimerQueue.h"
#include "base/Logging.h"
#include "Timer.h"
#include "TimerId.h"
#include "EventLoop.h"
#include "base/Timestamp.h"
#include <Callbacks.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <memory>
#include <assert.h>
#include <string.h>

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

TimerQueue::AddTimerInLoop::AddTimerInLoop(EventLoop *loop, Timer *timer, TimerQueue *tq) : loop_(loop), timer_(timer), tq_(tq) {}
TimerQueue::CancelTimerInLoop::CancelTimerInLoop(EventLoop *loop, Timer *timer, TimerQueue *tq) : loop_(loop), timer_(timer), tq_(tq) {}

void TimerQueue::AddTimerInLoop::someVoidCallback()
{
    loop_->assertInLoopThread();
    if (tq_->insert(timer_))
    {
        resetTimerfd(tq_->timerfd_, timer_->expiration()); //有一个时间很小定时器的加入了，那么就调用更新timerfd
    }
}

TimerId TimerQueue::addTimer(std::shared_ptr<Callbacks> timeCallback, Timestamp when, double interval)
{
    Timer *timer = new Timer(timeCallback, when, interval);
    std::shared_ptr<Callbacks> callback = std::make_shared<AddTimerInLoop>(loop_, timer, this);

    CallbackNum::NumCall path{CallbackNum::KVoid, callback};
    loop_->runInLoop(path);
}
