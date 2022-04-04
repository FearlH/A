#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "base/Logging.h"
#include "base/CurrentThread.h"
// #include "Poller.h"
//不能在这个地方引入头文件
//会使得编译的时候把Poller引入进来
//然后Poller又会引入EventLoop
//但是这个时候已经是在EventLoop头文件里面了
//那么这个EventLoop里面就包含的Poller就不会包含EventLoop
//导致之后的Poller使用EventLoop就是失效的
//就需要后面的前置声明，而不是这个时候引入头文件
#include <assert.h>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
namespace m2
{
    namespace net
    {
        class Channel;
        class Poller;
        class TimerId;
        class TimeQueue;
        class EventLoop
        {

            //别的线程执行的注册操作可以转移到本线程去执行
            // EventLoop里面存放Thread ID
            //可以判断是不是在当前的线程执行任务
        public:
            EventLoop();
            ~EventLoop();
            EventLoop(const EventLoop &) = delete;
            EventLoop &operator=(const EventLoop &) = delete;
            void loop();
            bool isInLoopThread() { return threadId_ == CurrentThread::tid(); } //是否在Loop线程里面
            void assertInLoopThread()
            {
                if (!isInLoopThread())
                {
                    abortNotInLoopThread();
                }
            }

            void quit(); //不是完全线程安全，可能需要使用智能指针

            ///
            /// Time when poll returns, usually means data arrival.
            ///
            Timestamp pollReturnTime() const { return pollReturnTime_; }

            int64_t iteration() const { return iteration_; }

            // / 在Loop里面执行一个任务
            // / 然后会唤醒这个Loop，执行任务
            void runInLoop(CallbackNum::NumCall taskCallback);
            /// Queues callback in the loop thread.
            /// Runs after finish pooling.
            /// Safe to call from other threads.
            //在这个线程里面运行这个函数，就是假设是在此线程里面运行的，那么就会直接运行
            //如果是在别的线程里面调用的，那么会使得任务被加入到这个Loop线程的一个队列里面
            //然后会唤醒这个Loop，执行任务
            //任务加入到Loop线程去做
            void queueInLoop(CallbackNum::NumCall taskCallback);

            size_t queueSize() const;

            // timers

            ///
            /// Runs callback at 'time'.
            /// Safe to call from other threads.
            ///
            TimerId runAt(Timestamp time, CallbackNum::NumCall taskCallback);
            ///
            /// Runs callback after @c delay seconds.
            /// Safe to call from other threads.
            ///
            TimerId runAfter(double delay, CallbackNum::NumCall taskCallback);
            ///
            /// Runs callback every @c interval seconds.
            /// Safe to call from other threads.
            ///
            TimerId runEvery(double interval, CallbackNum::NumCall taskCallback);
            ///
            /// Cancels the timer.
            /// Safe to call from other threads.
            ///
            void cancel(TimerId timerId);

            // internal usage
            void wakeup(); //异步唤醒

            // Channel使用了...................................
            void updateChannel(Channel *channel); //
            void removeChannel(Channel *channel); //
            bool hasChannel(Channel *channel);    //
            // Channel使用了...................................

            // // bool callingPendingFunctors() const { return callingPendingFunctors_; }
            bool eventHandling() const { return eventHandling_; }

            // void setContext(const boost::any &context)
            // {
            //     context_ = context;
            // }

            // const boost::any &getContext() const
            // {
            //     return context_;
            // }

            // boost::any *getMutableContext()
            // {
            //     return &context_;
            // }

            // static EventLoop *getEventLoopOfCurrentThread();

        private:
            class HandleWakeUpRead : public Callbacks
            {
            public:
                explicit HandleWakeUpRead(EventLoop *loop) : loop_(loop) {}
                void readCallback() override;

            private:
                EventLoop *loop_;
            };
            std::shared_ptr<Callbacks> handelRead_ = std::make_shared<HandleWakeUpRead>(this);
            bool looping_;
            const pid_t threadId_; //对象属于的线程的id

            void abortNotInLoopThread();
            // void handleRead();        // waked up，异步唤醒使用
            void doPendingNumCalls(); //异步唤醒之后需要作的工作

            // void printActiveChannels() const; // DEBUG

            typedef std::vector<Channel *> ChannelList;

            bool looping_; /* atomic */
            std::atomic<bool> quit_;
            bool eventHandling_;        /* atomic */
            bool doingPendingNumCalls_; /* atomic */
            int64_t iteration_;
            Timestamp pollReturnTime_;
            std::unique_ptr<Poller> poller_; // new出来的指针交给poller_监管
            std::unique_ptr<TimerQueue> timerQueue_;
            int wakeupFd_;
            // // unlike in TimerQueue, which is an internal class,
            // // we don't expose Channel to client.
            std::unique_ptr<Channel> wakeupChannel_;
            // boost::any context_;

            // // scratch variables
            ChannelList activeChannels_;
            Channel *currentActiveChannel_;
            mutable std::mutex mutex_;                          //异步加入需要锁
            std::vector<CallbackNum::NumCall> pendingNumCalls_; //异步加入的队列
        };
    }
}

#endif