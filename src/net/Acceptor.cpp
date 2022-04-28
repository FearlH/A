#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/Channel.h"
#include "net/SocketOps.h"
#include "net/InetAddress.h"
#include "base/Logging.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

using namespace m2;
using namespace m2::net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop),
      acceptorSocket_(socket::createNonblockingOrDie(listenAddr.family())),
      acceptorChannel_(loop, acceptorSocket_.fd()),
      listening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))

{
    assert(idleFd_ > 0);
    acceptorSocket_.setReuseAddr(true);
    acceptorSocket_.setReusePort(true);
    acceptorSocket_.bindAddress(listenAddr);
    acceptorChannel_.setReadCallback([this](Timestamp t)
                                     {
                                         this->loop_->assertInLoopThread();
                                         InetAddress peerAddr;
                                         int connfd = this->acceptorSocket_.accept(&peerAddr);
                                         if (connfd >= 0)
                                         {
                                             if (this->newConnectionCallback_)
                                             {
                                                 this->newConnectionCallback_(connfd, peerAddr);
                                             }
                                             else
                                             {
                                                 close(connfd);
                                             }
                                         }
                                         else
                                         {
                                             LOG_SYSERR << "in Acceptor::accept handleRead";
                                             if (errno == EMFILE)
                                             {
                                                 ::close(this->idleFd_);
                                                 int connfd = ::accept(this->acceptorSocket_.fd(), nullptr, nullptr);
                                                 ::close(connfd);
                                                 this->idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
                                             }
                                         }
                                     });
}
Acceptor::~Acceptor()
{
    acceptorChannel_.disableAll(); //会从poll里面解除自己
    acceptorChannel_.remove();     //再去从EventLoop里面移除自己
    ::close(idleFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    acceptorSocket_.listen();
    acceptorChannel_.enableReading();
    listening_ = true;
}