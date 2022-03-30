#include "ThreadPool.h"
#include <exception>
#include <string>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <stdlib.h>

using namespace m2;

ThreadPool::ThreadPool(const std::string &name, size_t poolSize, size_t maxTaskSize)
    : name_(name), poolSize_(poolSize), maxTaskSize_(maxTaskSize)
{
}
void ThreadPool::start()
{
    std::lock_guard<std::mutex> locker(mtx_);
    for (int i = 0; i < poolSize_; ++i)
    {
        std::unique_ptr<std::thread> uptrThread = std::make_unique<std::thread>(&ThreadPool::runInThread, this);
        uptrThreads_.emplace_back(std::move(uptrThread));
    }
    if (poolSize_ == 0 && initThreadCallback_)
    {
        initThreadCallback_->run(); //就在这个线程上面执行
    }
}
void ThreadPool::run(std::unique_ptr<Task> &&uptrTask)
{
    if (poolSize_ == 0)
    { //没有线程就在当前线程运行
        std::unique_ptr<Task> thisTask = std::move(uptrTask);
        thisTask->run();
    }
    else
    {
        std::unique_lock<std::mutex> ulock(mtx_);
        //满了会阻塞加入的线程
        fullCond_.wait(ulock, [&]
                       { return (maxTaskSize_ == 0 || taskNum_ < maxTaskSize_) || !running_; });
        if (!running_)
        {
            return;
        }
        uptrTasks_.push_back(std::move(uptrTask));
        ++taskNum_;
        emptyCond_.notify_one();
    }
}

void ThreadPool::run(std::shared_ptr<Task> task)
{
    if (poolSize_ == 0)
    { //没有线程就在当前线程运行
        std::shared_ptr<Task> thisTask = task;
        thisTask->run();
    }
    else
    {
        std::unique_lock<std::mutex> ulock(mtx_);
        //满了会阻塞加入的线程
        fullCond_.wait(ulock, [&]
                       { return (maxTaskSize_ == 0 || taskNum_ < maxTaskSize_) || !running_; });
        if (!running_)
        {
            return;
        }
        sptrTasks_.push_back(task);
        ++taskNum_;
        emptyCond_.notify_one();
    }
}

void ThreadPool::setMaxTaskSize(size_t newSize)
{
    std::lock_guard<std::mutex> locker(mtx_);
    maxTaskSize_ = newSize;
}
void ThreadPool::stop()
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        if (running_ == false)
        {
            return;
        }
        running_ = false;
        emptyCond_.notify_all();
        fullCond_.notify_all();
    }
    for (int i = 0; i < poolSize_; ++i)
    {
        if (uptrThreads_[i]->joinable())
        {
            uptrThreads_[i]->join();
        }
    }
}
void ThreadPool::setThreadInitCallback(std::unique_ptr<Task> &&uptrTask)
{
    std::lock_guard<std::mutex> locker(mtx_);
    initThreadCallback_ = std::move(uptrTask);
}

std::unique_ptr<Task> ThreadPool::takeFromUptr()
{
    std::unique_ptr<Task> task;
    std::unique_lock<std::mutex> ulock(mtx_);
    emptyCond_.wait(ulock, [&]
                    { return taskNum_ || !running_; });
    if (!uptrTasks_.empty())
    {
        task = std::move(uptrTasks_.front());
        uptrTasks_.pop_front();
        fullCond_.notify_one();
        --taskNum_;
    }
    return task;
}
std::shared_ptr<Task> ThreadPool::takeFromSptr()
{
    std::shared_ptr<Task> task;
    std::unique_lock<std::mutex> ulock(mtx_);
    emptyCond_.wait(ulock, [&]
                    { return taskNum_ || !running_; });
    if (!sptrTasks_.empty())
    {
        task = sptrTasks_.front();
        sptrTasks_.pop_front();
        fullCond_.notify_one();
        --taskNum_;
    }
    return task;
}
void ThreadPool::runInThread()
{
    try
    {
        if (initThreadCallback_)
        {
            initThreadCallback_->run();
        }
        while (running_)
        {
            std::unique_lock<std::mutex> ulock(mtx_);
            bool fromWhitch = whitch_;
            whitch_ = !whitch_;
            ulock.unlock();
            if (fromWhitch)
            {
                std::shared_ptr<Task> task = takeFromSptr();
                if (task)
                {
                    task->run();
                }
            }
            else
            {
                std::unique_ptr<Task> task(takeFromUptr());
                if (task)
                {
                    task->run();
                }
            }
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "ThreadPool thread error";
        abort();
    }
}
