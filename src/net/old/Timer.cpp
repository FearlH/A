#include "Timer.h"

using namespace m2;
using namespace net;

std::atomic<int64_t> Timer::s_creatNum_;

void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}