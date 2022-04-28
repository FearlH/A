#include "EventLoop.h"
#include "TimerQueue.h"
#include "base/Timestamp.h"
#include "TimerId.h"
#include <iostream>

int main()
{
    using namespace m2;
    using namespace net;
    EventLoop loop;
    TimerQueue q(&loop);
    TimerId id1 = q.addTimer([]
                             { std::cout << 1 << std::endl; },
                             Timestamp::now(), 1);
    TimerId id2 = q.addTimer([]
                             { std::cout << 2 << std::endl; },
                             Timestamp::now(), 1);
    loop.loop();
}