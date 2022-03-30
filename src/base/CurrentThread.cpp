#include "CurrentThread.h"
#include <unistd.h>
using namespace m2;
pid_t CurrentThread::pid()
{
    static pid_t thisPid = 0;
    if (thisPid == 0)
    {
        thisPid = ::getpid();
    }
    return thisPid;
}
pid_t CurrentThread::tid()
{
    thread_local pid_t thisTid = 0;
    if (thisTid == 0)
    {
        thisTid = ::gettid();
    }
    return thisTid;
}