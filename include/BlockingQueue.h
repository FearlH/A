#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <mutex>
#include <condition_variable>
#include <deque>
#include <assert.h>

namespace m2
{
    template <typename T>
    class BlockingQueue
    {

    public:
        using queue_type = std::deque<T>;
        using size_type = typename queue_type::size_type;
        BlockingQueue() = default;
        BlockingQueue(const BlockingQueue &) = delete;
        BlockingQueue &operator=(const BlockingQueue &) = delete;

        void put(const T &x)
        {
            std::unique_lock<std::mutex> ulocker(mtx_);
            queue_.push_back(x);
            ulocker.unlock();
            notEmpty_.notify_one(); //唤醒一个？
        }
        void put(T &&x)
        {
            std::unique_lock<std::mutex> ulocker(mtx_);
            queue_.push_back(x);
            notEmpty_.notify_one();
        }
        T take()
        {
            std::unique_lock<std::mutex> ulocker(mtx_);
            notEmpty_.wait(ulocker, [&]
                           { return !queue_.empty(); }); //条件成立就会返回
            T front(std::move(queue_.front()));          //可以采取移动的方法
            queue_.pop_front();
            return front;
        }

        queue_type drain() //排空
        {
            queue_type queue;
            {
                std::lock_guard<std::mutex> locker(mtx_);
                queue = std::move(queue_);
                assert(queue_.empty());
            }
            return queue;
        }
        size_type size() const
        {
            std::lock_guard<std::mutex> locker(mtx_);
            return queue_.size();
        }

    private:
        mutable std::mutex mtx_;
        std::condition_variable notEmpty_;
        queue_type queue_;
    };
} // m2

#endif // BLOCKINGQUEUE_H