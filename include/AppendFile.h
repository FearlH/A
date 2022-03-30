#ifndef APPENDFILE_H
#define APPENDFIEL_H

#include <stdio.h>
#include <string>
#include <memory>
namespace m2
{
    namespace FileDetail
    {
        class AppendFile //这个类就在锁里面使用
        {
        public:
            explicit AppendFile(const std::string &fileName);
            AppendFile(const AppendFile &) = delete;
            AppendFile &operator=(const AppendFile &) = delete;

            void append(const char *buf, size_t len);
            void flush();

            off_t writtenBytes() const { return writtenBytes_; }

            ~AppendFile();

        private:
            FILE *fp_;
            char IOBuffer_[64 * 1024]; //自己的设置的缓冲区
            off_t writtenBytes_;
        };
    } // detail
} // m2

#endif // APPENDFILE_H