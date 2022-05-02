#include "echo_client.h"
#include <iostream>
#include <functional>
#include <thread>
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoClient::EchoClient(EventLoop *loop, const InetAddress &addr)
    : loop_(loop), client_(loop_, addr, "aName")
{
    client_.setMessageCallback(std::bind(&EchoClient::onMessage, this, _1, _2, _3));
}
void EchoClient::connect()
{
    client_.connect();
    
}

void EchoClient::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    std::cout << buffer->retrieveAllAsString();
}

void EchoClient::doit()
{
    while (true)
    {
        TcpConnectionPtr ptr;
        {
            std::lock_guard<std::mutex> locker(mutex_);
            ptr = client_.connection();
        }
        std::string message;
        std::cin >> message;
        ptr->send(message);
    }
}

void EchoClient::run()
{
    if (!t_)
    {
        t_.reset(new std::thread(&EchoClient::doit, this));
    }
}
