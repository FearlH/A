#include "net/Poller.h"
#include "net/EpollPoller.h"

using namespace m2;
using namespace net;

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    return new EpollPoller(loop);
}

