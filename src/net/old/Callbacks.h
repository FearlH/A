#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "base/Timestamp.h"
namespace m2
{
    namespace net
    {
        class Channel;
        class Callbacks
        {
            friend class Channel;

        public:
            Callbacks();
            ~Callbacks();
            // virtual void eventCallback() = 0;
            // virtual void readCallback(Timestamp) = 0;
            // virtual void writeCallback() = 0;
            // virtual void timeCallback() = 0;
            // virtual void errorCallback() = 0;
            // virtual void closeCalback() = 0;
            const int fd() const { return fd_; }
            const Channel *bindChannel() { return thisChannel_; }
            virtual void eventCallback();
            virtual void readCallback(Timestamp);
            virtual void writeCallback();
            virtual void timeCallback();
            virtual void errorCallback();
            virtual void closeCalback();

        private:
            Channel *thisChannel_;
            int fd_ = -1;
        };
    }
}

#endif // CALLBACKS_H