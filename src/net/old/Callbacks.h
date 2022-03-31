#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "Timestamp.h"
namespace m2
{
    namespace net
    {
        class Callbacks
        {
        public:
            Callbacks();
            ~Callbacks();
            virtual void eventCallback() = 0;
            virtual void readCallback(Timestamp) = 0;
            virtual void writeCallback() = 0;
            virtual void timeCallback() = 0;
            virtual void errorCallback() = 0;
            virtual void closeCalback() = 0;
        };
    }
}

#endif // CALLBACKS_H