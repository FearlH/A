#include "echo_server.h"
int main()
{
    EventLoop *loop = new EventLoop();
    InetAddress addr(10000);
    EchoServer server(loop, addr);
    server.start();
    loop->loop();
}