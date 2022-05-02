#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include "base/Logging.h"
#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"

using namespace m2;
using namespace net;
class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr);
    void start();

private:
    void onConnection(const TcpConnectionPtr &conn);
    void onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time);   
    EventLoop *loop_;
    TcpServer server_;
};

#endif