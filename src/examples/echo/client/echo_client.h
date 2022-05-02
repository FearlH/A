#ifndef ECHO_CLIENT_H
#define ECHO_CLIENT_H
#include "net/EventLoop.h"
#include "net/TcpCilent.h"
#include <mutex>
#include <memory>
#include <thread>

using namespace m2;
using namespace net;
class EchoClient
{
public:
    EchoClient(EventLoop *loop, const InetAddress &addr);
    void connect();
    void doit();
    void run();
    ~EchoClient()
    {
        if (t_)
        {
            t_->join();
        }
    }

private:
    void onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time);
    std::unique_ptr<std::thread> t_;
    std::mutex mutex_;
    EventLoop *loop_;
    TcpClient client_;
};
#endif
