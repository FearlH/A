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
            explicit TimerQueue(EventLoop *);
            ~TimerQueue();
            TimerId addTimer(std::shared_ptr<Callbacks> timeCallback, Timestamp when, double interval);
            //是在这个TimeQueue里面构造，不是构造好之后传入
            //返回给用户一个TimerId
            void cancel(TimerId timeId);

        private:
            using Entry = std::pair<Timestamp, Timer *>;
            using ActiveTimer = std::pair<Timer *, int64_t>;
            using ActiveTimerSet = std::set<ActiveTimer>;

            // void addTimerInLoop(Timer *timer);
            // void cancelTimerInLoop(TimerId timerId);

            class AddTimerInLoop : public Callbacks
            {
                AddTimerInLoop(EventLoop *loop, Timer *timer, TimerQueue *tq);
                void someVoidCallback() override;

            private:
                EventLoop *loop_;
                Timer *timer_;
                TimerQueue *tq_;
            };
            class CancelTimerInLoop : public Callbacks
            {
                CancelTimerInLoop(EventLoop *loop, Timer *timer, TimerQueue *tq);
                void someVoidCallback() override;

            private:
                EventLoop *loop_;
                Timer *timer_;
                TimerQueue *tq_;
            };
            //使用一个TimeFd来计时，那么使用这个函数处理读取TimerFd
            void hadleRead();

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
            bool callExpeiredTimers_;
            ActiveTimerSet cancelTimets_;
        };
    }
}

#endif