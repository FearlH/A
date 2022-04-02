#ifndef SINGLETON_H
#define SINGLETON_H

#include <mutex>
#include <stdlib.h>

namespace m2
{
    template <typename T>
    class Singleton
    {
    public:
        Singleton() = delete;
        ~Singleton() = delete;
        Singleton(const Singleton &) = delete;
        Singleton &operator=(const Singleton &) = delete;

        static T &instance()
        {
            std::call_once(initOnceFlag_, init);
            return *value_;
        }

    private:
        static void init()
        {
            value_ = new T();
            ::atexit(destory);
        }
        static void destory()
        {
            using T_must_be_complete_type = char[sizeof(T) == 0 ? -1 : 1];
            //不完整类型也可以定义指针，但是不能析构，这就是定义了一个数组，如果T是一个不完整的类型
            //那么就会产生一个负数大小的数组
            T_must_be_complete_type dummy;
            delete value_;
            value_ = nullptr;
        }

        static std::once_flag initOnceFlag_;
        static T *value_;
        //不能直接定义对象
        // static T value;
        //这样的是可以保证只创建一个对象，但是不是new出来的，就不能去在程序运行的时候析构析构
        //析构函数会作出一些事情
    };

    // 类内部的static的对象需要在类的外部进行设置初值
    template <typename T>
    std::once_flag Singleton<T>::initOnceFlag_;

    template <typename T>
    T *Singleton<T>::value_ = nullptr;

    template <typename T, typename... Args>
    T &singleInstance(Args &&...args)
    {
        static T value(std::forward<Args>(args)...);
        return value;
    };

} // m2

#endif // SINGLETON_H
