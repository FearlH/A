#include "net/EventLoop.h"
#include "net/TcpConnection.h"
#include "net/TcpServer.h"
#include "net/InetAddress.h"
#include "base/Timestamp.h"

#include <functional>
#include <string>
#include <unordered_map>

void onMessage(const std::unordered_map<std::string, std::string> &m, const m2::net::TcpConnectionPtr &conn, m2::net::Buffer *buf, m2::Timestamp t)
{
    const char *CRLF = buf->findCRLF();
    if (CRLF)
    {
        std::string message(buf->peek(), CRLF);
        buf->retrieveUntil(CRLF + 2);
        if (m.find(message) != m.end())
        {
            std::string s = m.at(message);
        }
        else
        {
            conn->send("NOT FIND\n");
        }
        conn->shutdown();
    }
}

int main()
{
    using m2::net::EventLoop;
    using m2::net::InetAddress;
    using m2::net::TcpServer;
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    EventLoop *loop = new EventLoop();
    InetAddress addr(10000);
    TcpServer server(loop, addr, "finger");
    std::unordered_map<std::string, std::string> m;
    m["a"] = "aa";
    server.setMessageCallback(std::bind(onMessage, m, _1, _2, _3));
    server.start();
    loop->loop();
}