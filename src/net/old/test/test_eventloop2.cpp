#include "EventLoop.h"
#include "Channel.h"
#include "Callbacks.h"
#include <unistd.h>
#include "base/Timestamp.h"
#include <iostream>
#include <sys/timerfd.h>
#include <sys/time.h>
#include <string.h>
#include <memory>
#include <sys/epoll.h>
#include <iostream>
using namespace m2;
using namespace net;

class A : public Callbacks
{
    //电平触发，需要自己读取数据，不然就会一直触发
    void readCallback(Timestamp) override
    {
        char c[100];
        read(fd(), c, 100);
        std::cout << 111 << std::endl;
    }
};
int main()
{
    EventLoop loop;
    Channel c(&loop, 1);
    std::unique_ptr<Callbacks> call = std::make_unique<A>();
    c.setCallbacks(std::move(call));
    c.enableReading();
    loop.loop();
    // int *i;
    // struct epoll_event ev;
    // ev.events = EPOLLIN;
    // ev.data.ptr = i;
    // int epfd = epoll_create1(EPOLL_CLOEXEC);
    // epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);
    // epoll_event evs[10];
    // int res = epoll_wait(epfd, evs, 10, 10);
    // std::cout << res << std::endl;
}