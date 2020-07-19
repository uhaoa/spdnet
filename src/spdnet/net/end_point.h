#ifndef SPDNET_NET_END_POINT_H
#define SPDNET_NET_END_POINT_H

#include <memory>
#include <cstring>
#include <array>
#include <cassert>
#include <spdnet/base/buffer.h>

namespace spdnet {
    namespace net {

        class end_point {
        public:
            end_point() = default;

            ~end_point() = default;

            static end_point ipv4(const std::string &ip, const uint16_t port) {
                end_point ret;
                ::memset(&ret.addr_, 0, sizeof(ret.addr_));
                ret.addr_.sin_family = AF_INET;
                ret.addr_.sin_port = ::htons(port);
                ::inet_pton(AF_INET, ip.c_str(), &ret.addr_.sin_addr);
                return ret;
            }

            static end_point ipv6(const std::string &ip, const uint16_t port) {
                end_point ret;
                ::memset(&ret.addr6_, 0, sizeof(ret.addr6_));
                ret.addr6_.sin6_family = AF_INET6;
                ret.addr6_.sin6_port = ::htons(port);
                ::inet_pton(AF_INET6, ip.c_str(), &ret.addr6_.sin6_addr);
                return ret;
            }

            std::string ip() const {
                if (addr_.sin_family == AF_INET) {
                    char buf[INET_ADDRSTRLEN] = "";
                    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, INET_ADDRSTRLEN);
                    return std::string(buf);
                } else if (addr_.sin_family == AF_INET6) {
                    char buf[INET6_ADDRSTRLEN] = "";
                    ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, INET6_ADDRSTRLEN);
                    return std::string(buf);
                }
                assert(false);
                return std::string("unkown ip");
            }

            // support ipv4 and ipv6
            uint16_t family() const {
                return addr_.sin_family;
            }

            // support ipv4 and ipv6
            int16_t port() const {
                return ntohs(addr_.sin_port);
            }

            // support ipv4 and ipv6
            const struct sockaddr *
            socket_addr() const { return static_cast<const struct sockaddr *>((const void *) (&addr6_)); }

            // support ipv4 and ipv6
            size_t socket_addr_len() const { return sizeof(sockaddr_in6); };
        private:
            union {
                struct sockaddr_in addr_;
                struct sockaddr_in6 addr6_;
            };
        };

    }
}

#endif //SPDNET_NET_END_POINT_H
