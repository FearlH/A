#ifndef ASYNCLOG_H
#define ASYNCLOG_H

#include "CountDownLatch.h"
#include "BlockingQueue.h"
#include "LogStream.h"
#include "ProcessInfo.h"

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <memory>

namespace m2
{
    class AsyncLogging
    {

    public:
        AsyncLogging(const std::string &baseName, int flushTimeval_, off_t rollFileBytes);
        AsyncLogging(const AsyncLogging &) = delete;
        AsyncLogging &operator=(const AsyncLogging &) = delete;

        void append(const char *logLine, size_t len);

        void start();
        void stop();
        ~AsyncLogging();

    private:
        void threadFunc();

        std::mutex mtx_;
        std::condition_variable cond_;

        CountDownLatch latch_;

        const std::string baseName_;
        bool running_;
        int flushTimeval_;
        off_t rollSize_;

        using Buffer = m2::detail::FixedBuffer<m2::detail::kLargeBuffer>;
        using BufferPtr = std::unique_ptr<Buffer>;
        using BufferPtrVector = std::vector<BufferPtr>;

        BufferPtr currentBuffer_;
        BufferPtr nextBuffer_;

        BufferPtrVector bufferVector_;

        std::unique_ptr<std::thread> writeThread_;
    };
} // m2

#endif // ASYNCLOG_H