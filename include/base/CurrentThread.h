#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

#include <sys/types.h>

namespace m2
{
    namespace CurrentThread
    {
        pid_t pid();
        pid_t tid();
    } // namespace CurrentThread
} // namespace m2

#endif