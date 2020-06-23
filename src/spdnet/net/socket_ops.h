#ifndef SPDNET_BASE_SOCKETAPI_H_
#define SPDNET_BASE_SOCKETAPI_H_

#include <spdnet/base/platform.h>
#include <string>

namespace spdnet::net::socket_ops {
    int socketNoDelay(int fd);

    bool socketBlock(int fd);

    bool socketNonBlock(int fd);

    int socketSendBufSize(int fd, int size);

    int socketRecvBufSize(int fd, int size);

    void closeSocket(int fd);

    std::string getIPFromSockaddr(const struct sockaddr *from);

    std::string getIPFromSockFd(int fd);

    bool checkSelfConnect(int fd);
}

#endif // SPDNET_BASE_SOCKETAPI_H_