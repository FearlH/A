#include "base/Timestamp.h"
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

using namespace m2;

static_assert(sizeof(int64_t) == sizeof(Timestamp), "Timestamp should be same size of int64_t");

Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    int64_t microseconds = tv.tv_usec + tv.tv_sec * KMicrosecondsPerSecond;
    return Timestamp(microseconds);
}

std::string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microsecondsFromEpoch_ / KMicrosecondsPerSecond;
    int64_t microseconds = microsecondsFromEpoch_ % KMicrosecondsPerSecond;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64, seconds, microseconds);
    return buf;
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[64] = {0};
    time_t secnods = static_cast<time_t>(microsecondsFromEpoch() / KMicrosecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&secnods, &tm_time);
    if (showMicroseconds)
    {
        int microseconds = static_cast<int>(microsecondsFromEpoch() % KMicrosecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}