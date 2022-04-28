#ifndef SOCKET_H
#define SOCKET_H

struct tcp_info;

namespace m2
{
    namespace net
    {
        class InetAddress;

        //包装一个Socket文件描述符，在析构的时候就会关闭，所有的操作都被委托给OS，都是系统调用，就是线程安全的
        class Socket
        {
        public:
            explicit Socket(int sockfd) : sockfd_(sockfd) {}
            ~Socket();
            Socket(const Socket &) = delete;
            Socket &operator=(const Socket &) = delete;
            int fd() const { return sockfd_; }

            bool getTcpInfo(struct tcp_info *) const;
            bool getTcpInfoString(char *buf, int len) const;

            void bindAddress(const InetAddress &localAddress);
            void listen();
            int accept(InetAddress *peerAddress);
            void shutdownWrite();

            ///
            /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
            ///
            void setTcpNoDelay(bool on);

            ///
            /// Enable/disable SO_REUSEADDR
            ///
            void setReuseAddr(bool on);

            ///
            /// Enable/disable SO_REUSEPORT
            ///
            void setReusePort(bool on);

            ///
            /// Enable/disable SO_KEEPALIVE
            ///
            void setKeepAlive(bool on);

        private:
            const int sockfd_;
        };

    }
}

#endif // SOCKET_H