#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "InetAddress.h"

#include <functional>
#include <memory>

namespace m2
{
    namespace net
    {
        class Channel;
        class EventLoop;
        class Connector : public std::enable_shared_from_this<Connector>
        {
        public:
            using NewConnectionCallback = std::function<void(int sockfd)>;
            Connector(EventLoop *loop, const InetAddress &serverAddress);
            ~Connector();
            Connector(const Connector &) = delete;
            Connector &operator=(const Connector &) = delete;
            void setNewConnectionCallback(const NewConnectionCallback &callback) { newConnectionCallback_ = callback; }

            void start();   //任意线程
            void restart(); //必须是本线程loop_线程调用
            void stop();    //任意线程

            const InetAddress &serverAddress() const { return serverAddress_; }

        private:
            enum States
            {
                kDisconnected,
                kConnecting,
                kConnected
            };
            static const int KMaxRetryDelayMs = 30 * 1000;
            static const int KInitRetryDelayMs = 500;

            void setState(States s) { state_ = s; }
            void startInLoop();
            void stopInLoop();
            void connect();
            void connecting(int sockfd);
            void handleWrite();
            void handleError();
            void retry(int sockfd);
            int removeAndResetChannel();
            void resetChannel();

            EventLoop *loop_;
            InetAddress serverAddress_;
            bool connect_;
            States state_;
            std::unique_ptr<Channel> channel_;
            NewConnectionCallback newConnectionCallback_;
            int retryDelayMs_;
        };
    }
}

#endif // CONNECTOR_H