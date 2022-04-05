#ifndef TIMER_H
#define TIMER_H

#include "base/Timestamp.h"
#include "Callbacks.h"
#include <atomic>
#include <memory>

namespace m2
{
    namespace net
    {
        //只有时间的设定没有说明如何计时
        //存放了过期时间 定时器，然后和Callback
        class Timer
        {
        public:
            Timer(VoidFunc timeCallback, Timestamp when, double interval)
                : timeCallback_(timeCallback), expiration_(when),
                  interval_(interval), repeat_(interval > 0),
                  thisNumSeq_(s_creatNum_.fetch_add(1)) {}
            void run()
            {
                timeCallback_();
            }
            Timestamp expiration() const { return expiration_; }
            bool repeat() const { return repeat_; }
            int64_t sequence() const { return thisNumSeq_; }
            void restart(Timestamp now);
            static int64_t numCreated() { return s_creatNum_.load(); }

        private:
            VoidFunc timeCallback_;                  // Callback
            Timestamp expiration_;                   //到期时间
            const double interval_;                  //时间间隔,有时间间隔就是会重复，没有就是不会重复
            const bool repeat_;                      //是否重复
            const int64_t thisNumSeq_;               //唯一的编号
            static std::atomic<int64_t> s_creatNum_; //递增的标号，每一个新建的Timer都会使用一个独一无二的标号
        };
    }
}
#endif