#ifndef SPDNET_BASE_END_POINT_H
#define SPDNET_BASE_END_POINT_H

#include <memory>
#include <cstring>
#include <array>
#include <cassert>
#include <spdnet/base/buffer.h>

namespace spdnet::base {


    class EndPoint {
    public:
        static EndPoint ipv4(const std::string& ip , const uint16_t port)
        {
            EndPoint ret;
            ret.addr_.sin_family = AF_INET;
            ret.addr_.sin_port = port;
            ::inet_pton(AF_INET, ip, &ret.addr_.sin_addr)
        }

        static EndPoint ipv6(const std::string& ip , const uint16_t port)
        {
            EndPoint ret;
            ret.addr6_.sin6_family = AF_INET6;
            ret.addr6_.sin6_port = port ;
            ::inet_pton(AF_INET6, ip, &ret.addr6_.sin6_addr) ;
        }

        string ip() const
        {
            if (addr_.sa_family == AF_INET)
            {
                char buf[INET_ADDRSTRLEN] = "";
                ::inet_ntop(AF_INET, &addr_.sin_addr, buf, INET_ADDRSTRLEN);
                return buf;
            }
            else if (addr_.sa_family == AF_INET6)
            {
                char buf[INET6_ADDRSTRLEN] = "";
                ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, INET6_ADDRSTRLEN);
                return buf;
            }
            assert(false);
            return "unkown ip" ;
        }

        uint16_t port() const
        {
            return addr_.sin_port;
        }
        const struct sockaddr* getSockAddr() const { return static_cast<struct sockaddr*>(&addr6_); }
    private:
        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
    };

}

#endif //SPDNET_BASE_END_POINT_H
