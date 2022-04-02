#include "base/RollFile.h"
#include "base/ProcessInfo.h"

using namespace m2;

RollFile::RollFile(const std::string &baseName,
                   off_t rollFileEdgeBytes,
                   bool threadSave,
                   int flushTimeVal,
                   int checkCount)
    : baseName_(baseName),
      rollFileEdgeBytes_(rollFileEdgeBytes),
      flushTimeval_(flushTimeVal),
      checkCount_(checkCount),
      count_(0),
      lastFlush_(0),
      lastRoll_(0),
      startRollFromEpoch_(0),
      mutex_(threadSave ? std::make_unique<std::mutex>() : nullptr)

{
    roll();
}

bool RollFile::roll()
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> locker(*mutex_);
        return roll_unlocked();
    }
    else
    {
        return roll_unlocked();
    }
}

void RollFile::append(const char *buf, size_t len)
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> locker(*mutex_);
        append_unlocked(buf, len);
    }
    else
    {
        append_unlocked(buf, len);
    }
}

void RollFile::append_unlocked(const char *buf, size_t len)
{
    file_->append(buf, len);
    if (file_->writtenBytes() > rollFileEdgeBytes_)
    {
        roll_unlocked();
    }
    else
    {
        ++count_;
        if (count_ > checkCount_)
        {
            //没有写满就看时间有没有到时间
            count_=0;
            time_t now = ::time(nullptr);
            time_t start = now / KSecondPerDay * KSecondPerDay;
            if (start > startRollFromEpoch_)
            {
                roll_unlocked();
            }
            else if (now - lastFlush_ >= flushTimeval_)
            {
                lastFlush_ = now;
                file_->flush(); // flush()会死锁
            }
        }
    }
}

bool RollFile::roll_unlocked()
{
    time_t now = 0;
    std::string fileName = getFileName(baseName_, &now);
    time_t start = now / KSecondPerDay * KSecondPerDay;
    if (now > lastRoll_)
    {
        startRollFromEpoch_ = start;
        lastRoll_ = now;
        lastFlush_ = now;
        file_.reset(new FileDetail::AppendFile(fileName));
        return true;
    }
    return false;
}

std::string RollFile::getFileName(const std::string &baseName, time_t *now)
{
    std::string fileName;
    fileName.reserve(baseName.size() + 64);
    fileName += baseName;
    char timebuf[32];
    struct tm tm;
    *now = ::time(nullptr);
    gmtime_r(now, &tm); // FIXME: localtime_r ?
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    fileName += timebuf;

    fileName += ProcessInfo::hostName();

    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    fileName += pidbuf;

    fileName += ".log";

    return fileName;
}

void RollFile::flush()
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> locker(*mutex_);
        time_t now = ::time(nullptr);
        lastFlush_ = now;
        file_->flush();
    }
    else
    {
        time_t now = ::time(nullptr);
        lastFlush_ = now;
        file_->flush();
    }
}


RollFile::~RollFile()
{
    file_->flush();
}
