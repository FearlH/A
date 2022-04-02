#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace m2
{
    class noncopyable
    {
    public:
        noncopyable(const noncopyable &) = delete;
        noncopyable &operator=(const noncopyable &) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
} // m2

#endif // NONCOPYABLE_H
