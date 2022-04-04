#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "base/Timestamp.h"
#include <map>
#include <memory>
#include <functional>
namespace m2
{

    template <typename To, typename From>
    inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From> &f)
    {
        if (false)
        {
            implicit_cast<From *, To *>(0);
        }

        // #ifndef NDEBUG
        //         assert(f == NULL || dynamic_cast<To *>(get_pointer(f)) != NULL);
        // #endif
        return ::std::static_pointer_cast<To>(f);
    }
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
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
            virtual void readCallback();
            virtual void someVoidCallback();

        private:
            Channel *thisChannel_;
            int fd_ = -1;
        };
        namespace CallbackNum
        {
            const int64_t KEvent = 1;
            const int64_t KReadTime = KEvent * 2;
            const int64_t KWrite = KReadTime * 2;
            const int64_t KTime = KWrite * 2;
            const int64_t KError = KTime * 2;
            const int64_t KClose = KError * 2;
            const int64_t KRead = KClose * 2;
            const int64_t KVoid = KRead * 2;
            using NumCall = std::pair<int64_t, std::shared_ptr<Callbacks>>;
            NumCall callNumPair(const int64_t todo, std::shared_ptr<Callbacks> callback);
            void doCallNumPair(NumCall &pCalls, Timestamp time = Timestamp::invalid());
        }
    }
}

#endif // CALLBACKS_H