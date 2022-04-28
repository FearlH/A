#include "net/Poller.h"
#include "net/Channel.h"

using namespace m2;
using namespace net;

Poller::Poller(EventLoop *loop) : ownerLoop_(loop)
{
}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel *channel) const
{
    assetInLoopThread();
    ChannelMap::const_iterator it = fd_Channels_.find(channel->fd());
    return it != fd_Channels_.end() && it->second == channel;
}
