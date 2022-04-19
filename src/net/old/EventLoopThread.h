#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>

namespace m2
{
    namespace net
    {
        class EventLoopThread
        {
        public:
            using ThreadFunc = std::function<void()>;
            using ThreadInitCallback = std::function<void(EventLoop *)>;
            EventLoopThread(const ThreadInitCallback &callback = ThreadInitCallback(), const const std::string &name = std::string());
            ~EventLoopThread();
            EventLoop *strat();

        private:
            std::mutex mutex_;
            std::condition_variable cond_;
            ThreadInitCallback callback_;
            EventLoop *loop_;
            bool exiting_; //没有这个析构的时候不知道是不是开启了loop
            std::thread loopThread_;
            ThreadFunc threadFunc_;
            std::string name_;
        };
    }
}