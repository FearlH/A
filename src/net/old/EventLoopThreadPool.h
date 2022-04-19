#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace m2
{
    namespace net
    {
        class EventLoopThread;
        class EventLoop;

        class EventLoopThreadPool
        {
            //构造函数就是会传递一个基础的EventLoop进去，然后再根据是不是需要pool构造别的
        public:
            using ThreadInitCallback = std::function<void(EventLoop *)>;
            EventLoopThreadPool(EventLoop *baseLoop, const std::string &name);
            ~EventLoopThreadPool();
            EventLoopThreadPool(const EventLoopThreadPool &) = delete;
            EventLoopThreadPool &operator=(const EventLoopThreadPool &) = delete;

            void setThreadNum(int threadNum) { numThreads_ = threadNum; }
            void start(const ThreadInitCallback &callback = ThreadInitCallback());
            EventLoop *getNextLoop();
            EventLoop *getLoopForHash(size_t hashcode);
            std::vector<EventLoop *> getAllLoops();
            bool started() const { return started_; }
            const std::string &name() { return name_; }

        private:
            EventLoop *baseLoop_; //至少的一个
            std::string name_;
            bool started_;
            int numThreads_;
            int next_;
            std::vector<std::unique_ptr<EventLoopThread>> threads_;
            std::vector<EventLoop *> loops_;
        };
    }
}

#endif // EVENTLOOPTHREADPOOL_H