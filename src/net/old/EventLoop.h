#ifndef OLD_EVENTLOOP_H
#define OLD_EVENTLOOP_H

#include "Logging.h"
#include "CurrentThread.h"
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
namespace m2
{
    namespace net
    {
        class Channel;
        class Poller;
        class EventLoop
        {
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

            // ///
            // /// Time when poll returns, usually means data arrival.
            // ///
            // Timestamp pollReturnTime() const { return pollReturnTime_; }

            // int64_t iteration() const { return iteration_; }

            // /// Runs callback immediately in the loop thread.
            // /// It wakes up the loop, and run the cb.
            // /// If in the same loop thread, cb is run within the function.
            // /// Safe to call from other threads.
            // void runInLoop(Functor cb);
            // /// Queues callback in the loop thread.
            // /// Runs after finish pooling.
            // /// Safe to call from other threads.
            // void queueInLoop(Functor cb);

            // size_t queueSize() const;

            // // timers

            // ///
            // /// Runs callback at 'time'.
            // /// Safe to call from other threads.
            // ///
            // TimerId runAt(Timestamp time, TimerCallback cb);
            // ///
            // /// Runs callback after @c delay seconds.
            // /// Safe to call from other threads.
            // ///
            // TimerId runAfter(double delay, TimerCallback cb);
            // ///
            // /// Runs callback every @c interval seconds.
            // /// Safe to call from other threads.
            // ///
            // TimerId runEvery(double interval, TimerCallback cb);
            // ///
            // /// Cancels the timer.
            // /// Safe to call from other threads.
            // ///
            // void cancel(TimerId timerId);

            // // internal usage
            // void wakeup();

            // Channel使用了...................................
            void updateChannel(Channel *channel); //
            void removeChannel(Channel *channel); //
            bool hasChannel(Channel *channel);    //
            // Channel使用了...................................

            // // pid_t threadId() const { return threadId_; }
            // void assertInLoopThread()
            // {
            //     if (!isInLoopThread())
            //     {
            //         abortNotInLoopThread();
            //     }
            // }
            // bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
            // // bool callingPendingFunctors() const { return callingPendingFunctors_; }
            // bool eventHandling() const { return eventHandling_; }

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
            bool looping_;
            const pid_t threadId_; //对象属于的线程的id

            void abortNotInLoopThread();
            // void handleRead(); // waked up
            // void doPendingFunctors();

            // void printActiveChannels() const; // DEBUG

            typedef std::vector<Channel *> ChannelList;

            // bool looping_; /* atomic */
            std::atomic<bool> quit_;
            bool eventHandling_; /* atomic */
            // bool callingPendingFunctors_; /* atomic */
            int64_t iteration_;
            Timestamp pollReturnTime_;
            std::unique_ptr<Poller> poller_; // new出来的指针交给poller_监管
            // std::unique_ptr<TimerQueue> timerQueue_;
            // int wakeupFd_;
            // // unlike in TimerQueue, which is an internal class,
            // // we don't expose Channel to client.
            std::unique_ptr<Channel> wakeupChannel_;
            // boost::any context_;

            // // scratch variables
            ChannelList activeChannels_;
            Channel *currentActiveChannel_;

            // mutable MutexLock mutex_;
            // std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);
        };
    }
}

#endif