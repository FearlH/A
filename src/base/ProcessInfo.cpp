#include "ProcessInfo.h"

using namespace m2;

pid_t ProcessInfo ::pid()
{
    return ::getpid();
}

std::string ProcessInfo::pidString()
{
    return std::to_string(pid());
}

std::string ProcessInfo::hostName()
{
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0)
    {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    else
    {
        return "unknownhost";
    }
}