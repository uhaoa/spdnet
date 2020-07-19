#ifndef SPDNET_NET_SOCKET_OPS_H_
#define SPDNET_NET_SOCKET_OPS_H_

#include <spdnet/base/platform.h>
#include <string>
#include <iostream>

namespace spdnet {
    namespace net {
        namespace socket_ops {

            inline int socket_no_delay(sock_t fd) {
                int on = 1;
                return ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *) &on, sizeof(on));
            }

            inline bool socket_block(sock_t fd) {
                int err;
                unsigned long ul = true;
#ifdef SPDNET_PLATFORM_WINDOWS
                err = ioctlsocket(fd, FIONBIO, &ul);
#else
                err = ioctl(fd, FIONBIO, &ul);
#endif
                return err != SPDNET_SOCKET_ERROR;
            }

            inline bool socket_non_block(sock_t fd) {
                int err;
                unsigned long ul = true;
#ifdef SPDNET_PLATFORM_WINDOWS
                err = ioctlsocket(fd, FIONBIO, &ul);
#else
                err = ioctl(fd, FIONBIO, &ul);
#endif

                return err != SPDNET_SOCKET_ERROR;
            }

            inline int socket_send_buf_size(sock_t fd, int size) {
                return ::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *) &size, sizeof(size));
            }

            inline int socket_recv_buf_size(sock_t fd, int size) {
                return ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *) &size, sizeof(size));
            }

            inline void close_socket(sock_t fd) {
#ifdef SPDNET_PLATFORM_WINDOWS
                ::closesocket(fd);
#else
                ::close(fd);
#endif
            }

            inline std::string get_ip_from_sockaddr(const struct sockaddr *from) {
#ifdef SPDNET_PLATFORM_WINDOWS
                using AddrType = PVOID;
#else
                using AddrType = const void *;
#endif
                char tmp[INET6_ADDRSTRLEN] = {0};
                switch (from->sa_family) {
                    case AF_INET:
                        inet_ntop(AF_INET, (AddrType) (&(((const struct sockaddr_in *) from)->sin_addr)),
                                  tmp, sizeof(tmp));
                        break;
                    case AF_INET6:
                        inet_ntop(AF_INET6, (AddrType) (&(((const struct sockaddr_in6 *) from)->sin6_addr)),
                                  tmp, sizeof(tmp));
                        break;
                }

                return tmp;
            }

            inline std::string get_ip_from_sockfd(sock_t fd) {
#ifdef SPDNET_PLATFORM_WINDOWS
                struct sockaddr name = sockaddr();
                int namelen = sizeof(name);
                if (::getpeername(fd, (struct sockaddr*)&name, &namelen) == 0)
                {
                    return get_ip_from_sockaddr(&name);
                }
#else
                struct sockaddr_in name = sockaddr_in();
                socklen_t namelen = sizeof(name);
                if (::getpeername(fd, (struct sockaddr *) &name, &namelen) == 0) {
                    return get_ip_from_sockaddr((const struct sockaddr *) &name);
                }
#endif

                return "";
            }


            inline static struct sockaddr_in6 get_peer_addr(sock_t sockfd) {
                struct sockaddr_in6 peer_addr = sockaddr_in6();
                auto addrlen = static_cast<socklen_t>(sizeof peer_addr);
                if (::getpeername(sockfd, (struct sockaddr *) (&peer_addr), &addrlen) < 0) {
                    return peer_addr;
                }
                return peer_addr;
            }

            inline static struct sockaddr_in6 get_local_addr(sock_t sockfd) {
                struct sockaddr_in6 local_addr = sockaddr_in6();
                auto addrlen = static_cast<socklen_t>(sizeof local_addr);
                if (::getsockname(sockfd, (struct sockaddr *) (&local_addr), &addrlen) < 0) {
                    return local_addr;
                }
                return local_addr;
            }

            inline bool check_self_connect(sock_t fd) {
                struct sockaddr_in6 localaddr = get_local_addr(fd);
                struct sockaddr_in6 peeraddr = get_peer_addr(fd);

                if (localaddr.sin6_family == AF_INET) {
                    const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localaddr);
                    const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peeraddr);
                    return laddr4->sin_port == raddr4->sin_port
                           && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
                } else if (localaddr.sin6_family == AF_INET6) {
#ifdef SPDNET_PLATFORM_WINDOWS
                    return localaddr.sin6_port == peeraddr.sin6_port
                        && memcmp(&localaddr.sin6_addr.u.Byte,
                            &peeraddr.sin6_addr.u.Byte,
                            sizeof localaddr.sin6_addr.u.Byte) == 0;
#else
                    return localaddr.sin6_port == peeraddr.sin6_port
                           && memcmp(&localaddr.sin6_addr.s6_addr,
                                     &peeraddr.sin6_addr.s6_addr,
                                     sizeof localaddr.sin6_addr.s6_addr) == 0;
#endif
                } else {
                    return false;
                }
            }

            inline void clear_errno() {
#if defined(SPDNET_PLATFORM_WINDOWS)
                WSASetLastError(0);
#else
                errno = 0;
#endif
            }

            inline sock_t create_socket(int family, int type, int protocol) {
#if defined(SPDNET_PLATFORM_WINDOWS)
                sock_t s = ::WSASocketW(family, type, protocol, 0, 0,
                    WSA_FLAG_OVERLAPPED);
                if (s == invalid_socket)
                    return s;

                if (family == AF_INET6)
                {
                    // reference asio
                    // Try to enable the POSIX default behaviour of having IPV6_V6ONLY set to
                    // false. This will only succeed on Windows Vista and later versions of
                    // Windows, where a dual-stack IPv4/v6 implementation is available.
                    DWORD optval = 0;
                    ::setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,
                        reinterpret_cast<const char*>(&optval), sizeof(optval));
                }
                return s;
#else
                sock_t s = ::socket(family, type, protocol);
                if (s == invalid_socket)
                    return s;

                /*
                int optval = 1;
                int result = ::setsockopt(s,
                    SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));
                if (result != 0)
                {
                    ::close(s);
                    return invalid_socket;
                }
                */
                return s;
#endif
            }
        }
    }
}
#endif // SPDNET_NET_SOCKET_OPS_H_