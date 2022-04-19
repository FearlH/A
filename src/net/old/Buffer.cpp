#include "Buffer.h"
#include "SocketOps.h"

#include <errno.h>
#include <sys/uio.h>

using namespace m2;
using namespace m2::net;

const char Buffer::KCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

/*
buffer需要尽量大，才能有较大的吞吐量
但是太大会使得每个TcpConnection的Buffer空间初始化就分配的过大
那么会使得有很多无用的空间会占用很多的内存
使用一个extraBuffer，使得每一次读取的时候都会有一个比较大的buffer
吞吐量大
同时同一个线程内的大家共享同一个extraBuffer(原来的muduo源码使用的是栈上面的临时的extraBuffer,函数调用完就自动释放了)
读取完成之后还会使得每一个buffer的大小都是合适的，没有过多的内存分配
*/
const size_t KExtraBufferSize = 65536;
thread_local char extraBuffer[KExtraBufferSize]; // ip数据报最大长度65535
//因为对于Buffer的操作都会在创建Buffer的那个 IO线程里面调用
//所以使用一个thread_local的extraBuffer不会发生竞争

ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    struct iovec vec[2];
    const size_t BytesWritable = writableBytes();
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = BytesWritable;
    vec[1].iov_base = extraBuffer;
    vec[1].iov_len = KExtraBufferSize;
    //?muduo上面写的是如果buffer的空间足够大(大于等于extraBuffer的空间，那么就只使用buffer的空间)
    const int iovcnt = (BytesWritable >= KExtraBufferSize ? 1 : 2);
    const ssize_t n = socket::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (implicit_cast<size_t>(n) <= BytesWritable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extraBuffer, n - BytesWritable);
    }
    return n;
}