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
            //只不过是这个类里面包含了一个Channel和一个Socket
            // Channel可读就是有连接进来了
            //然后接受连接，之后会调用ConnectionCallback去创建一个TcpServer的对象
            //EventLoop里面还是通过Channel和Pool来实现Reactor的
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
            Socket acceptorSocket_;   //对于socket的一个封装，里面有一些函数，就是bind,listen,setsockopt之类的
            Channel acceptorChannel_; //把这个Channel注册到EventLoop里面了
            NewConnectionCallback newConnectionCallback_;
            bool listening_;
            int idleFd_; //用来当文件描述符满了之后accept的
        };
    }
}

#endif // ACCEPTOR_H