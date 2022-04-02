#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>
#include <string>

namespace m2
{
    class Task
    {
    public:
        virtual void run() = 0;
        virtual ~Task() {}
        // operator bool() { return emptyTask_; }
        // bool setEmptyTask(bool isEmpty) { emptyTask_ = isEmpty; }

    private:
        // bool emptyTask_ = true;
    };

    class ThreadPool
    {

    public:
        ThreadPool(const std::string &name, size_t poolSize = 2, size_t maxTaskSize = 20);
        void start();
        void run(std::unique_ptr<Task> &&uptrTask);
        void run(std::shared_ptr<Task> sptrTask);
        void stop();
        void setMaxTaskSize(size_t newSize);
        // void setMaxTaskSize(size_t maxSize);
        void setThreadInitCallback(std::unique_ptr<Task> &&uptrTask);

        const std::string poolName() const { return name_; }
        const size_t maxTaskSize() const { return maxTaskSize_; }
        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;
        ~ThreadPool() { stop(); }

    private:
        std::unique_ptr<Task> takeFromUptr();
        std::shared_ptr<Task> takeFromSptr();
        void runInThread();

        mutable std::mutex mtx_;
        std::condition_variable fullCond_;
        std::condition_variable emptyCond_;
        std::unique_ptr<Task> initThreadCallback_;
        std::vector<std::unique_ptr<std::thread>> uptrThreads_; //保存这个线程
        std::deque<std::unique_ptr<Task>> uptrTasks_;
        std::deque<std::shared_ptr<Task>> sptrTasks_;
        size_t maxTaskSize_; // 0就是无限大
        size_t poolSize_;
        size_t taskNum_ = 0;
        std::string name_;
        bool running_ = true;
        bool whitch_ = true;
    };
} // m2
#endif // THREADPOOL_H