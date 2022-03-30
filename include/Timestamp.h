#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <inttypes.h>
#include <string>

namespace m2
{
    class Timestamp
    {
    public:
        explicit Timestamp(int64_t microsecondsFromEpochArgs) : microsecondsFromEpoch_(microsecondsFromEpochArgs) {}
        Timestamp() : microsecondsFromEpoch_(0) {}
        //可以拷贝移动
        //使用默认生成的函数
        void swap(Timestamp &that)
        {
            std::swap(microsecondsFromEpoch_, that.microsecondsFromEpoch_);
        }
        std::string toString() const;
        std::string toFormattedString(bool showMicroseconds = true) const;
        bool valid() const { return KMicrosecondsPerSecond > 0; }
        int64_t microsecondsFromEpoch() const { return microsecondsFromEpoch_; }
        time_t secondsFromEpoch() const
        {
            return static_cast<time_t>(microsecondsFromEpoch_ / KMicrosecondsPerSecond); //需要类型转换不然不一样
        }
        static Timestamp now();    //获取现在的时间
        static Timestamp invalid() //获取一个无效的时间
        {
            return Timestamp();
        }
        static Timestamp fromUnixTime(time_t t)
        {
            return fromUnixTime(t, 0);
        };
        static Timestamp fromUnixTime(time_t t, int microseconds)
        {
            return Timestamp(t * KMicrosecondsPerSecond + microseconds);
        }
        static const int KMicrosecondsPerSecond = 1000 * 1000;

        bool operator<(const Timestamp &that)
        {
            return microsecondsFromEpoch() < that.microsecondsFromEpoch();
        }
        bool operator==(const Timestamp &that)
        {
            return microsecondsFromEpoch() == that.microsecondsFromEpoch();
        }

    private:
        int64_t microsecondsFromEpoch_; //从纪元以来的微秒数
    };

    inline bool operator<(const Timestamp &t1, const Timestamp &t2)
    {
        return t1 < t2;
    }

    inline bool operator==(const Timestamp &t1, const Timestamp &t2)
    {
        return t1 == t2;
    }

    //得到时间的差，按照秒来返回
    inline double timeDifference(const Timestamp &t1, const Timestamp &t2)
    {
        int64_t diff = t1.microsecondsFromEpoch() - t2.microsecondsFromEpoch();
        return static_cast<double>(diff) / Timestamp::KMicrosecondsPerSecond;
    }

    //给指定的时间增加秒数,返回新的时间
    inline Timestamp addTime(const Timestamp &time, double seconds)
    {
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::KMicrosecondsPerSecond); // delta 增量
        return Timestamp(time.microsecondsFromEpoch() + delta);
    }
} // m2
#endif //_TIMESTAMP_H_