#include "echo_client.h"
#include "net/EventLoop.h"
using namespace m2;
using namespace net;
int main()
{
    EventLoop *loop = new EventLoop();
    InetAddress addr("127.0.0.1", 10000);
    EchoClient client(loop, addr);
    client.connect();
    client.run();
    loop->loop();
}