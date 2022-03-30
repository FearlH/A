#include "AsyncLog.h"
#include "RollFile.h"
#include "Timestamp.h"
#include "ProcessInfo.h"

#include <chrono>
#include <stdio.h>

using namespace m2;

AsyncLogging::AsyncLogging(const std::string &baseName, int flushTimeval_, off_t rollFileBytes)

    : cond_(), mtx_(), latch_(1),
      baseName_(baseName), flushTimeval_(flushTimeval_), rollSize_(rollFileBytes),
      currentBuffer_(std::make_unique<Buffer>()), nextBuffer_(std::make_unique<Buffer>()), bufferVector_()
{
    currentBuffer_->reset();
    nextBuffer_->reset();
    bufferVector_.reserve(16);
}

void AsyncLogging::start()
{
    running_ = true;
    if (!writeThread_)
    {
        writeThread_ = std::move(std::make_unique<std::thread>(&AsyncLogging::threadFunc, this));
    }
    cond_.notify_one();
    latch_.wait();
}

void AsyncLogging::append(const char *logLine, size_t len)
{
    std::unique_lock<std::mutex> ulock(mtx_);
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logLine, len);
    }
    else
    {
        bufferVector_.push_back(std::move(currentBuffer_));
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            currentBuffer_ = std::make_unique<Buffer>();
        }
        currentBuffer_->append(logLine, len);
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc()
{
    latch_.countDown();
    RollFile logFile(baseName_, rollSize_, true, flushTimeval_);
    BufferPtr newBuffer1 = std::make_unique<Buffer>();
    BufferPtr newBuffer2 = std::make_unique<Buffer>();
    BufferPtrVector toWriteBuffers;
    toWriteBuffers.reserve(16);
    std::chrono::seconds oneSecond(1);
    while (running_)
    {
        {
            std::unique_lock<std::mutex> ulock(mtx_);
            if (bufferVector_.empty())
            {
                cond_.wait_for(ulock, oneSecond);
                // cond_.wait_for(ulock, oneSecond, [&]
                //            { !bufferVector_.empty(); });
                //这样的话，关闭的时候和开始的时候的notify会因为没到时间或者是不为空而阻塞
            }
            // toWriteBuffers.push_back(std::move(currentBuffer_));
            // for (auto &buf : bufferVector_)
            // {
            //     toWriteBuffers.push_back(std::move(buf));
            // }

            //交换比添加然后pop快
            bufferVector_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            toWriteBuffers.swap(bufferVector_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }
        if (toWriteBuffers.size() > 64)
        {
            //过多的写入，就会使得丢弃日志，只保留需要轮回的两个日志
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     Timestamp::now().toFormattedString().c_str(),
                     toWriteBuffers.size() - 2);
            fputs(buf, stderr);
            logFile.append(buf, 256);
            toWriteBuffers.erase(toWriteBuffers.begin() + 2, toWriteBuffers.end());
        }
        //这个时候就可以释放锁了
        for (const auto &buf : toWriteBuffers)
        {
            logFile.append(buf->data(), buf->length());
            logFile.flush();
        }
        logFile.flush();
        if (toWriteBuffers.size() > 2)
        {
            toWriteBuffers.resize(2);
            //去掉多余的，不然内存空间不会释放
        }
        if (!newBuffer1)
        {
            newBuffer1 = std::move(toWriteBuffers.back());
            toWriteBuffers.pop_back();
            newBuffer1->reset();
        }
        if (!newBuffer2)
        {
            newBuffer2 = std::move(toWriteBuffers.back());
            toWriteBuffers.pop_back();
            newBuffer2->reset();
        }
        toWriteBuffers.clear();
        logFile.flush();
    }
    logFile.flush();
}
