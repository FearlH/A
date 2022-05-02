#include "net/SocketOps.h"
#include "base/Logging.h"
#include "base/Types.h"
#include "net/Endian.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace m2;
using namespace m2::net;
using namespace m2::net::socket;

namespace
{
    using SA = sockaddr;
}

const struct sockaddr *socket::sockaddr_cast(const struct sockaddr_in *addr)
{
    return static_cast<const sockaddr *>((const void *)addr);
}

const struct sockaddr *socket::sockaddr_cast(const struct sockaddr_in6 *addr)
{
    return static_cast<const sockaddr *>((const void *)addr);
}

struct sockaddr *socket::sockaddr_cast(struct sockaddr_in6 *addr)
{
    return static_cast<sockaddr *>((void *)addr);
}

const struct sockaddr_in *socket::sockaddr_in_cast(const struct sockaddr *addr)
{
    return static_cast<const sockaddr_in *>((const void *)addr);
}

const struct sockaddr_in6 *socket::sockaddr_in6_cast(const struct sockaddr *addr)
{
    return static_cast<const sockaddr_in6 *>((const void *)addr);
}

int socket::createNonblockingOrDie(int family)
{
    int fd = ::socket(family, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP); //最后一个可以使用0替代
    if (fd < 0)
    {
        LOG_SYSFATAL << "CREAT SOCKET ERROR createNonblockingOrDie(int family)";
    }
    return fd;
}

int socket::connect(int sockfd, const struct sockaddr *addr)
{
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6))); //???为什么要这样写
}

void socket::bindOrDie(int sockfd, const struct sockaddr *addr)
{
    int res = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (res < 0)
    {
        LOG_SYSFATAL << "BIND ERROR bindOrDie(int sockfd, const struct sockaddr *addr)";
    }
}

void socket::listenOrDie(int sockfd)
{
    int res = ::listen(sockfd, SOMAXCONN);
    //?直接就是最大连接数，才4096，高并发几万是怎么来的
    //就只是队列的总数，不是连接的最大数量
    if (res < 0)
    {
        LOG_SYSFATAL << "LISTEN ERROR socket::listenOrDie";
    }
}

int socket::accept(int sockfd, struct sockaddr_in6 *addr)
{
    socklen_t socklen = sizeof(addr);
    int fd = accept4(sockfd, sockaddr_cast(addr), &socklen, SOCK_CLOEXEC | SOCK_NONBLOCK);
    if (fd < 0)
    {
        int savedErrno = errno;
        switch (savedErrno)
        {
        case EAGAIN:       //资源临时不可用，需要重复调用
        case ECONNABORTED: //软件引起的连接终止
        case EINTR:        //系统调用中断
        case EPROTO:       // ???协议错误
        case EPERM:        //权限不允许
        case EMFILE:       // 文件描述符不够用了 per-process lmit of open file desctiptor ???
            // expected errors
            errno = savedErrno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            // unexpected errors
            LOG_FATAL << "unexpected error of ::accept " << savedErrno;
            break;
        default:
            LOG_FATAL << "unknown error of ::accept " << savedErrno;
            break;
        }
    }
    return fd;
}

ssize_t socket::read(int sockfd, void *buf, size_t count)
{
    return ::read(sockfd, buf, count);
}
ssize_t socket::readv(int sockfd, const struct iovec *iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}
ssize_t socket::write(int sockfd, const void *buf, size_t count)
{
    return ::write(sockfd, buf, count);
}

void socket::close(int sockfd)
{
    ::close(sockfd);
}
void socket::shutdownWrite(int sockfd)
{
    ::shutdown(sockfd, SHUT_WR);
}
void socket::toIpPort(char *buf, size_t size,
                      const struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET6)
    {
        buf[0] = '[';
        toIp(buf + 1, size - 1, addr);
        size_t end = ::strlen(buf);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        uint16_t port = socket::networkToHost16(addr6->sin6_port);
        assert(size > end);
        snprintf(buf + end, size - end, "]:%u", port);
        return;
    }
    toIp(buf, size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    uint16_t port = socket::networkToHost16(addr4->sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, ":%u", port);
}
void socket::toIp(char *buf, size_t size,
                  const struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
    else if (addr->sa_family == AF_INET6)
    {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

void fromIpPort(const char *ip, uint16_t port,
                struct sockaddr_in *addr);
void fromIpPort(const char *ip, uint16_t port,
                struct sockaddr_in6 *addr);

int getSocketError(int sockfd);

void socket::fromIpPort(const char *ip, uint16_t port,
                        struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

void socket::fromIpPort(const char *ip, uint16_t port,
                        struct sockaddr_in6 *addr)
{
    addr->sin6_family = AF_INET6;
    addr->sin6_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
    {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

int socket::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

struct sockaddr_in6 socket::getLocalAddr(int sockfd)
{
    struct sockaddr_in6 localaddr;
    memZero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

struct sockaddr_in6 socket::getPeerAddr(int sockfd)
{
    struct sockaddr_in6 peeraddr;
    memZero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
    {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

bool socket::isSelfConnect(int sockfd)
{
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET)
    {
        const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localaddr);
        const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    }
    else if (localaddr.sin6_family == AF_INET6)
    {
        return localaddr.sin6_port == peeraddr.sin6_port && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    }
    else
    {
        return false;
    }
}
