#ifndef POLLER_H
#define POLLER_H

//在EventLoop.h里面包含这个头文件的时候
//下面这行因为有#define的存在就失效了
#include "EventLoop.h"
#include "Timestamp.h"

#include <vector>
#include <map>

namespace m2
{
    namespace net
    {
        class Channel; //前向声明
        // Poller负责对于Channel和Channel的一些设定
        class Poller //负责注册信号Epoll
        {
        public:
            using ChannelList = std::vector<Channel *>;
            Poller(EventLoop *loop);
            virtual ~Poller();
            //里面有一些操作
            void assetInLoopThread() const
            {
                ownerLoop_->assertInLoopThread();
            }

            /// Polls the I/O events.
            /// Must be called in the loop thread.
            virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
            // EventLoop会调用这个Pooler的poll函数,给一个时间，然后把这个Poller里面的就绪的Channel返回

            /// Changes the interested I/O events.
            /// Must be called in the loop thread.
            virtual void updateChannel(Channel *channel) = 0;

            /// Remove the channel, when it destructs.
            /// Must be called in the loop thread.
            virtual void removeChannel(Channel *channel) = 0;

            virtual bool hasChannel(Channel *channel) const; //这个就是这个Poller里面有这个Channel

            static Poller *newDefaultPoller(EventLoop *loop);

        protected:
            using ChannelMap = std::map<int, Channel *>;
            ChannelMap fd_Channels_;

        private:
            EventLoop *ownerLoop_;
        };
    }
}

#endif