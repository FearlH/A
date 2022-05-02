#include "echo_server.h"
#include "base/Logging.h"
#include <functional>
#include <string>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoServer::EchoServer(EventLoop *loop, const InetAddress &addr)
    : loop_(loop), server_(loop, addr, "aName")
{
    server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start()
{
    server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << "CONN";
}

void EchoServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    std::string message = buffer->retrieveAllAsString();
    conn->send(message);
}