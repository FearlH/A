#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "base/Timestamp.h"
#include "base/Types.h"
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
        class Buffer;
        class TcpConnection;
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using VoidFunc = std::function<void()>;
        using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                                   Buffer *buf,
                                                   Timestamp)>;
        using TimerCallback = std::function<void()>;

        using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
        using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
        using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

        void defaultConnectionCallback(const TcpConnectionPtr &conn);
        void defaultMessageCallback(const TcpConnectionPtr &conn,
                                    Buffer *buffer,
                                    Timestamp receiveTime);
    }
}

#endif // CALLBACKS_H