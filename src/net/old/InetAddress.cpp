#include "InetAddress.h"

#include "base/Logging.h"
#include "Endian.h"
#include "SocketOps.h"
#include "base/Types.h"

#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

static const in_addr_t KInAddrAny = INADDR_ANY;          // 0.0.0.0
static const in_addr_t KIAddrLoopback = INADDR_LOOPBACK; // 127.0.0.1

using namespace m2;
using namespace m2::net;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    if (ipv6)
    {
        memset(&addr6_, 0, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = socket::hostToNetwork16(port);
    }
    else
    {
        memset(&addr4_, 0, sizeof(addr4_));
        addr4_.sin_family = AF_INET;
        in_addr_t ip = (loopbackOnly ? KIAddrLoopback : KInAddrAny);
        addr4_.sin_addr.s_addr = socket::hostToNetwork32(ip);
        addr4_.sin_port = socket::hostToNetwork16(port);
    }
}

InetAddress::InetAddress(StringArg ip, uint16_t portArg, bool ipv6)
{
    if (ipv6 || strchr(ip.c_str(), ':'))
    {
        memZero(&addr6_, sizeof addr6_);
        socket::fromIpPort(ip.c_str(), portArg, &addr6_);
        //里面是inet_pton
    }
    else
    {
        memZero(&addr4_, sizeof addr4_);
        socket::fromIpPort(ip.c_str(), portArg, &addr4_);
    }
}
string InetAddress::toIpPort() const
{
    char buf[64] = "";
    socket::toIpPort(buf, sizeof buf, getSockAddr());
    return buf;
}

string InetAddress::toIp() const
{
    char buf[64] = "";
    socket::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

uint32_t InetAddress::ipv4NetEndian() const
{
    assert(family() == AF_INET);
    return addr4_.sin_addr.s_addr;
}

uint16_t InetAddress::port() const
{
    return socket::networkToHost16(portNetEndian());
}

static __thread char t_resolveBuffer[64 * 1024];

bool InetAddress::resolve(StringArg hostname, InetAddress *out)
{
    assert(out != NULL);
    struct hostent hent;
    struct hostent *he = NULL;
    int herrno = 0;
    memZero(&hent, sizeof(hent));

    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
    if (ret == 0 && he != NULL)
    {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        out->addr4_.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
        return true;
    }
    else
    {
        if (ret)
        {
            LOG_SYSERR << "InetAddress::resolve";
        }
        return false;
    }
}

void InetAddress::setScopeId(uint32_t scope_id)
{
    if (family() == AF_INET6)
    {
        addr6_.sin6_scope_id = scope_id;
    }
}
