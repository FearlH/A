#ifndef TIMERID_H
#define TIMERID_H
#include <inttypes.h>
namespace m2
{
    namespace net
    {
        class Timer;
        class TimerId //可以复制
        {
            friend class TimeSequence;

        public:
            TimerId() : timer_(nullptr), sequence_(0) {}
            TimerId(Timer *timer, int64_t seq) : timer_(timer), sequence_(seq) {}

        private:
            Timer *timer_;
            int64_t sequence_;
        };
    }
}

#endif