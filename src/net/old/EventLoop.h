#ifndef OLD_EVENTLOOP_H
#define OLD_EVENTLOOP_H

#include "Logging.h"
#include "CurrentThread.h"
#include <assert.h>
namespace m2
{
    namespace net
    {
        class EventLoop
        {
        public:
            EventLoop();
            ~EventLoop();
            EventLoop(const EventLoop &) = delete;
            EventLoop &operator=(const EventLoop &) = delete;
            void loop();
            bool isInLoopThread() { return threadId_ == CurrentThread::tid(); } //是否在Loop线程里面
            void assertInLoopThread()
            {
                if (!isInLoopThread())
                {
                    abortNotInLoopThread();
                }
            }

        private:
            void
            abortNotInLoopThread();

            bool looping_;
            const pid_t threadId_; //对象属于的线程的id
        };
    }
}

#endif