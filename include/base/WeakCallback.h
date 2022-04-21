#ifndef WEAKCALLBACK_H
#define WEAKCALLBACK_H

#include <memory>
#include <functional>

template <typename CLASS, typename... ARGS> //...就是把...之前的内容重复应用，扩展
class WeakCallback
{
    //对于一个对象，用一个weak_ptr绑定这个对象，然后还有这个对象的成员函数
    //如果对象存在，就可以调用这个对象的成员函数
public:
    WeakCallback(const std::weak_ptr<CLASS> &weakPtr, const std::function<void(CLASS *, ARGS...)> function)
        : object_(weakPtr),
          function_(function)
    {
    }
    void operator()(ARGS &&...args) const
    {
        std::shared_ptr<CLASS> objPtr = object_.lock();
        if (objPtr)
        {
            function_(objPtr.get(), std::forward<ARGS>(args)...);
            //成员函数....this指针..............参数................
        }
    }

private:
    std::weak_ptr<CLASS> object_;
    std::function<void(CLASS *, ARGS...)> function_;
};

// int (*pFunction)(double);

template <typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS> &sPtr, void (CLASS::*function)(ARGS...))
{
    return WeakCallback<CLASS, ARGS...>(sPtr, function);
}

template <typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS> &sPtr, void (CLASS::*function)(ARGS...) const)
{
    return WeakCallback<CLASS, ARGS...>(sPtr, function);
}

#endif // WEAKCALLBACK_H