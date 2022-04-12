#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "Channel.h"
#include "Socket.h"
#include <functional>

namespace m2
{
    namespace net
    {
        class EventLoop;
        class InetAddress;
        class Acceptor
        {
        public:
            using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
            Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
            ~Acceptor();
            void setNewConnectionCallBack(const NewConnectionCallback &callback)
            {
                newConnectionCallback_ = callback;
            }
            void listen();
            bool listening() { return listening_; }
            //这里面还能指定名字的

        private:
            EventLoop *loop_;
            Socket acceptorSocket_;
            Channel acceptorChannel_;
            NewConnectionCallback newConnectionCallback_;
            bool listening_;
            int idleFd_; //用来当文件描述符满了之后accept的
        };
    }
}

#endif // ACCEPTOR_H