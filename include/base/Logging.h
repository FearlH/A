#ifndef LOGGING_H
#define LOGGING_H

#include "LogStream.h"
#include "Timestamp.h"

#include <stdio.h>
#include <string.h>
namespace m2
{
    //构造的时候都是往里面填入一些信息，还有确认是不是达到了标准
    //然后后面的就是析构的时候向一个个global的对象里面写入
    class Logger
    {
    public:
        enum LogLevel
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            NUM_LOG_LEVELS,
        };
        class SourceFile
        {
        public:
            //__FILE__上面得到的是相对路径，会存在A/B/C这样的情况
            //这个SourceFile这个对象就是只获取文件名
            template <int SIZE>
            SourceFile(const char (&str)[SIZE]) : data_(str), size_(SIZE)
            {
                const char *slash = ::strrchr(data_, '/');
                if (slash)
                {
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - str);
                }
            }

            //muduo
            // template <int N>
            // SourceFile(const char (&arr)[N])
            //     : data_(arr),
            //       size_(N - 1)
            // {
            //     const char *slash = strrchr(data_, '/'); // builtin function
            //     if (slash)
            //     {
            //         data_ = slash + 1;
            //         size_ -= static_cast<int>(data_ - arr);
            //     }
            // }
            explicit SourceFile(const char *str) : data_(str), size_(strlen(data_))
            {
                const char *slash = ::strrchr(data_, '/');
                if (slash)
                {
                    data_ = slash + 1;
                    size_ = strlen(data_);
                }
            }
            const char *data_;
            int size_;
        };

        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char *func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();

        LogStream &stream() { return impl_.stream_; }

        static LogLevel logLevel(); //获取当前的LogLevel
        static void setLogLevel(LogLevel level);

        typedef void (*OutputFunc)(const char *msg, int len);
        typedef void (*FlushFunc)();
        static void setOutput(OutputFunc);
        static void setFlush(FlushFunc);

    private:
        class Impl
        {
        public:
            using LogLevel = Logger::LogLevel;
            Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
            void finish();
            void formatTime();
            SourceFile basename_;
            Timestamp time_;
            LogStream stream_;
            LogLevel level_;
            int line_;
        };
        Impl impl_;
    };

    extern Logger::LogLevel g_logLevel;

    inline Logger::LogLevel Logger::logLevel()
    {
        return g_logLevel;
    }

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//
#define LOG_TRACE                                    \
    if (m2::Logger::logLevel() <= m2::Logger::TRACE) \
    m2::Logger(__FILE__, __LINE__, m2::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                    \
    if (m2::Logger::logLevel() <= m2::Logger::DEBUG) \
    m2::Logger(__FILE__, __LINE__, m2::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                    \
    if (m2::Logger::logLevel() <= m2::Logger::INFO) \
    m2::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN m2::Logger(__FILE__, __LINE__, m2::Logger::WARN).stream()
#define LOG_ERROR m2::Logger(__FILE__, __LINE__, m2::Logger::ERROR).stream()
#define LOG_FATAL m2::Logger(__FILE__, __LINE__, m2::Logger::FATAL).stream()
#define LOG_SYSERR m2::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL m2::Logger(__FILE__, __LINE__, true).stream()

    const char *strerror_tl(int savedErrno);

    // Taken from glog/logging.h
    //
    // Check that the input is non NULL.  This very useful in constructor
    // initializer lists.

#define CHECK_NOTNULL(val) \
    ::m2::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

    // A small helper for CHECK_NOTNULL().
    template <typename T>
    T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
    {
        if (ptr == NULL)
        {
            Logger(file, line, Logger::FATAL).stream() << names;
        }
        return ptr;
    }

} // m2

#endif // LOGGING_H