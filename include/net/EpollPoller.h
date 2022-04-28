#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H

#include "Poller.h"

#include <sys/types.h>
#include <sys/epoll.h>
#include <vector>

namespace m2
{
    namespace net
    {
        class EpollPoller : public Poller
        {

        public:
            EpollPoller(EventLoop *loop);
            ~EpollPoller();
            /// Polls the I/O events.
            /// Must be called in the loop thread.
            Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
            // EventLoop会调用这个Pooler的poll函数,给一个时间，然后把这个Poller里面的就绪的Channel返回


            /// Changes the interested I/O events.
            /// Must be called in the loop thread.
            void updateChannel(Channel *channel) override; //对于这个Channel，去更新Poller里面的Epoll
            // Channel里面已经有了 event了，所以还需要知道Channel在Poller里面的状态，就需要Channel的index_

            /// Remove the channel, when it destructs.
            /// Must be called in the loop thread.
            void removeChannel(Channel *channel) override;

            // virtual bool hasChannel(Channel *channel) const; //这个就是这个Poller里面有这个Channel
        private:
            static const int KInitEventListSize_ = 30;
            void fillActiveChannleList(int numEvents,
                                       ChannelList *list) const;

            const char *operationToString(int op);
            void updateInEpoll(int options, Channel *); //对应于Epoll add del mod
            using EpollEventList = std::vector<struct epoll_event>;
            EpollEventList resultEvents_;
            int epollFd_;
        };
    }
}
#endif // EPOLLPOLLER