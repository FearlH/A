#include "AsyncLog.h"
#include <string>
#include <thread>

using namespace m2;
void func(AsyncLogging *l)
{
    while (1)
    {
        l->append("aa1", 4);
    }
}
int main()
{
    AsyncLogging *l = new AsyncLogging("name", 3, 1024);
    l->start();
    std::thread(func, l).join();
    std::thread(func, l).join();
}