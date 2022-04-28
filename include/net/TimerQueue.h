#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H
#include "base/Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"

#include <set>
#include <map>
#include <vector>
#include <memory>

namespace m2
{
    namespace net
    {
        class TimerId;
        class Timer;
        class EventLoop;

        // lambda表达式生成可调用对象还是有优势的
        //在别的线程里面有可能会发生回调函数
        //然后回调函数注册时间，那么就产生了不在IO线程注册计时器的情况
        class TimerQueue
        {
        public:
            explicit TimerQueue(EventLoop *loop);
            ~TimerQueue();
            TimerId addTimer(VoidFunc timeCallback, Timestamp when, double interval);
            //是在这个TimeQueue里面构造，不是构造好之后传入
            //返回给用户一个TimerId
            void cancel(TimerId timeId);

        private:
            using Entry = std::pair<Timestamp, Timer *>;
            using ActiveTimer = std::pair<Timer *, int64_t>;
            using ActiveTimerSet = std::set<ActiveTimer>;

            void addTimerInLoop(Timer *timer); //包装一个可调用对象，传入EventLoop
            void cancelTimerInLoop(TimerId timerId);

            //使用一个TimeFd来计时，那么使用这个函数处理读取TimerFd
            //处理计时的时候的超时的读取
            void hadleRead();
            //先把过期的都拿出来，然后执行，最后更新计时器
            // reset(expired, now); //过期的事件里面如果有重复的，那么就重新设置新的定时器

            // move out all expired timers
            std::vector<Entry> getExpired(Timestamp now);                 //获取超时的Timer
            void reset(const std::vector<Entry> &expired, Timestamp now); //把超时的记录重新设置

            bool insert(Timer *timer);

            EventLoop *loop_;
            const int timerfd_;
            Channel timerfdChannel_;
            // Timer list sorted by expiration

            std::set<Entry> timerList_;
            ActiveTimerSet activeTimers_; //这个和上面那个保存的是完全一样的内容，只不过排序方式不一样
            //? Timer的到期时间可能是会变的，所以可呢是需要两个set
            bool callingExpiredTimers_;
            ActiveTimerSet cancelTimers_;
        };
    }
}

#endif