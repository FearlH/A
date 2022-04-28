#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "TcpConnection.h"
#include "Callbacks.h"
#include <mutex>
#include <memory>
#include <string>

namespace m2
{
    namespace net
    {
        class Connector;
        class EventLoop;
        using ConnectorPtr = std::shared_ptr<Connector>;
        class TcpClient
        {
        public:
            TcpClient(EventLoop *loop, const InetAddress &serverAddr, const std::string &nameArg);
            ~TcpClient();

            TcpClient(const TcpClient &) = delete;
            TcpClient &operator=(const TcpClient &) = delete;

            void connect();
            void disconnect();
            void stop();

            TcpConnectionPtr connection() const { return connection_; }
            EventLoop *getLoop() const { return loop_; }
            bool retry() const { return retry_; }
            const string &name() { return name_; }

            /// Set connection callback.
            /// Not thread safe.
            void setConnectionCallback(ConnectionCallback cb)
            {
                connectionCallback_ = std::move(cb);
            }

            /// Set message callback.
            /// Not thread safe.
            void setMessageCallback(MessageCallback cb)
            {
                messageCallback_ = std::move(cb);
            }

            /// Set write complete callback.
            /// Not thread safe.
            void setWriteCompleteCallback(WriteCompleteCallback cb)
            {
                writeCompleteCallback_ = std::move(cb);
            }

        private:
            void newConnection(int sockfd);
            void removeConnection(const TcpConnectionPtr &connectionPtr);
            EventLoop *loop_;
            ConnectorPtr connector_;
            const std::string name_;
            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;
            bool retry_;
            bool connect_;

            int nextConnId_;
            mutable std::mutex mutex_;
            TcpConnectionPtr connection_;
        };
    }
}

#endif // TCPCLIENT_H