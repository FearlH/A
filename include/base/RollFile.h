#ifndef ROLLFILE_H
#define ROLLFILE_H

#include "AppendFile.h"
#include <time.h>
#include <unistd.h>
#include <memory>
#include <mutex>
#include <string>

namespace m2
{

    class RollFile
    {

    public:
        RollFile(const std::string &baseName, off_t rollFileEdgeBytes, bool threadSave = true, int flushTimeVal = 3, int checkCount = 1024);
        RollFile(const RollFile &) = delete;
        RollFile &operator=(const RollFile &) = delete;
        bool roll();
        void append(const char *buf, size_t len);
        void flush();
        ~RollFile();

    private:
        bool roll_unlocked(); //单纯roll不应该调用，是在锁里面调用的
        std::string getFileName(const std::string &baseName, time_t *now);
        void append_unlocked(const char *buf, size_t len);
        std::string baseName_;
        time_t lastRoll_;
        time_t lastFlush_;
        time_t startRollFromEpoch_;

        int flushTimeval_;        //刷新秒数
        off_t rollFileEdgeBytes_; //最大的写入的量，大于这个值就roll
        int checkCount_;

        int count_;

        std::unique_ptr<std::mutex>
            mutex_;

        std::unique_ptr<FileDetail::AppendFile> file_;

        const int KSecondPerDay = 24 * 60 * 60;
    };

}

#endif // ROLLFILE_H