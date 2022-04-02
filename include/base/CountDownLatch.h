#ifndef COUNTDOWNLATCH_H
#define COUNTDOWNLATCH_H
#include <mutex>
#include <condition_variable>

namespace m2
{

    class CountDownLatch //也是不可拷贝
    {
    public:
        explicit CountDownLatch(int count) : count_(count) {}
        void wait();
        void countDown();
        CountDownLatch &operator=(const CountDownLatch &) = delete;
        CountDownLatch(const CountDownLatch &) = delete;

    private:
        std::mutex mtx_;
        std::condition_variable cond_;
        int count_;
    };

} // m2

#endif // COUNTDOWNLATCH_H
