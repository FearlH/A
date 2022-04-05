#include "EventLoopThread.h"
#include "TimerQueue.h"
#include "base/Timestamp.h"
#include "TimerId.h"
#include <unistd.h>

#include <iostream>

int main()
{
    using namespace m2;
    using namespace net;
    EventLoopThread ev;
    EventLoop *loop = ev.strat();
    TimerId id1 = loop->runEvery(3, []
                                 { std::cout << 1 << std::endl; });
    TimerId id2 = loop->runEvery(3, []
                                 { std::cout << 2 << std::endl; });
    sleep(10);
    loop->cancel(id2);
    sleep(20);
}