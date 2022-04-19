#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "TcpConnection.h"
#include "base/Types.h"

#include <map>
#include <functional>
#include <string>
#include <atomic>

namespace m2
{
    namespace net
    {
        class Acceptor;
        class EventLoop;
        class EventLoopThreadPool;
        class TcpServer
        {
            //回调函数都是外部设置给内部的对象的
            //使用回调函数来编写库
            //实现调用反转
            //内部去写控制的流程，设置回调函数，写好调用的流程(类似于模板方法模式)
            //然后后面的高层次的对象去设置这个回调函数
        public:
            using ThreadInitCallback = std::function<void(EventLoop *)>;
            enum Opinion
            {
                KNoReusePort,
                KReusePort,
            };
            TcpServer(EventLoop *loop,
                      const InetAddress &listenAddr,
                      const std::string nameArg,
                      Opinion op = KReusePort);
            ~TcpServer();
            //返回一些值
            const string &ipPort() const { return ipPort_; }
            const string &name() const { return name_; }
            EventLoop *getLoop() const { return loop_; }

            void setThreadNum(int numThreads);
            void setThreadInitCallback(const ThreadInitCallback &callback)
            {
                threadInitCallback_ = callback;
            }
            std::shared_ptr<EventLoopThreadPool> threadPool()
            {
                return threadPool_;
            }

            void start(); //开始监听，是线程安全的

            //这些因该是在创建的线程里面调用的
            /// Set connection callback.
            /// Not thread safe.
            void setConnectionCallback(const ConnectionCallback &cb)
            {
                connectionCallback_ = cb;
            }

            /// Set message callback.
            /// Not thread safe.
            void setMessageCallback(const MessageCallback &cb)
            {
                messageCallback_ = cb;
            }

            /// Set write complete callback.
            /// Not thread safe.
            void setWriteCompleteCallback(const WriteCompleteCallback &cb)
            {
                writeCompleteCallback_ = cb;
            }

        private:
            //在loop里面是线程安全的
            //这个函数是在acceptor_里面调用的
            //需要改变一下自己的状态，所以还会去bind自己
            void newConnection(int sockfd, const InetAddress &peerAddress);

            //会调用线程安全的InLoop函数
            void removeConnection(const TcpConnectionPtr &conn);
            //在Loop里面是线程安全的
            void removeConnectionInLoop(const TcpConnectionPtr &conn);

            using ConnectionMap = std::map<string, TcpConnectionPtr>;

            EventLoop *loop_; //用来listen的loop
            const std::string ipPort_;
            const std::string name_;
            //拥有这个对象，就去设置这个对象的回调函数
            std::unique_ptr<Acceptor> acceptor_;
            std::shared_ptr<EventLoopThreadPool> threadPool_;

            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;
            ThreadInitCallback threadInitCallback_;

            std::atomic<int> started_; //??这个为什么需要原子操作

            int nextConnId_;
            ConnectionMap connections_;
        };
    }
}

#endif // TCPSERVER_H