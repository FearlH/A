#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <sys/types.h>
#include <unistd.h>
#include <string>

namespace m2
{
    namespace ProcessInfo
    {
        pid_t pid();
        std::string pidString();
        std::string hostName();
    } // ProcessInfo
} // m2

#endif // PROCESSINFO_H