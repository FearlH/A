#ifndef CHANNEL_H
#define CHANNEL_H
#include "Callbacks.h"
#include "EventLoop.h"
#include <memory>
#include <functional>

namespace m2
{
    namespace net
    {
        class Channel
        {

        public:
            Channel(EventLoop *loop, int fd);
            ~Channel();
            Channel(const Channel &) = delete;
            Channel &operator=(const Channel &) = delete;

            void handleEvent(Timestamp receiveTime);
            //??? use functional

            using EventCallback = std::function<void()>;
            using ReadEventCallback = std::function<void(Timestamp)>;

            void setReadCallback(ReadEventCallback rCallback)
            {
                //复制完成之后移动，相当于是把复制进来的那个参数移动了
                readCallback_ = std::move(rCallback);
            }
            void setWriteCallbacks(EventCallback wCallback)
            {
                writeCallback_ = std::move(wCallback);
            }
            void setErrorCallbacks(EventCallback eCallback)
            {
                errorCallback_ = std::move(eCallback);
            }
            void setCloseCallbacks(EventCallback cCallback)
            {
                closeCallback_ = std::move(cCallback);
            }

            /// Tie this channel to the owner object managed by shared_ptr,
            /// prevent the owner object being destroyed in handleEvent.
            void tie(const std::shared_ptr<void> &);
            //在Channel执行的时候把自己的owner用shared_ptr绑定，防止执行的时候自己的拥有着析构了

            int fd() const { return fd_; }
            int events() const { return events_; }
            void set_revents(int revt) { revents_ = revt; }            // used by pollers
            bool isNoneEvent() const { return events_ == kNoneEvent; } //注册的是啥都没有那就是不需要观察了，可以从Epoll里面移除

            void enableReading()
            {
                events_ |= kReadEvent;
                update();
            }
            void disableReading()
            {
                events_ &= ~kReadEvent;
                update();
            }
            void enableWriting()
            {
                events_ |= kWriteEvent;
                update();
            }
            void disableWriting()
            {
                events_ &= ~kWriteEvent;
                update();
            }
            void disableAll()
            {
                events_ = kNoneEvent;
                update();
            }

            bool isReading() const
            {
                return events_ & kReadEvent;
            }
            bool isWriting() const
            {
                return events_ & kWriteEvent;
            }
            // poller使用
            int index() { return index_; }
            void setIndex(int index) { index_ = index; }

            // for debug
            string reventsToString() const;
            string eventsToString() const;

            void doNotLogHup() { logHup_ = false; }

            EventLoop *ownerLoop() { return loop_; }
            void remove(); //调用的是loop的remove

        private:
            static string eventsToString(int fd, int ev); //用于打印调试信息的

            void update();                                    //更新消息
            void handleEventWithGuard(Timestamp receiveTime); //在tie之后处理事物

            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;
            // channnel就相当于是把文件描述符还有事件还有回调函数还有loop连接起来的东西
            //这些东西设计的时候都是在同一个线程里面的，所以裸指针也行？

            EventLoop *loop_; //所属的EeventLoop
            const int fd_;    //文件描述符，但是不负责关闭文件描述符
            int events_;      //需要注册的事件,注册的是啥都没有那就是不需要观察了，可以从Epoll里面移除
            int revents_;     //返回的事件
            int index_;       //表明一个channel在Poll数组里面的状态
            bool logHup_;

            std::weak_ptr<void> tie_;
            bool tied_;
            bool eventHandling_;
            bool addedToLoop_;

            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;
            ReadEventCallback readCallback_;
        };
    }
}

#endif // CHANNEL_H