#ifndef SPDNET_BASE_SOCKETAPI_H_
#define SPDNET_BASE_SOCKETAPI_H_

#include <spdnet/base/platform.h>
#include <string>

namespace spdnet::base {
    // 禁用Nagle算法
    int socketNoDelay(int fd);

    // 设置阻塞模式
    bool socketBlock(int fd);

    // 设置非阻塞模式
    bool socketNonBlock(int fd);

    // 设置socket内核发送缓冲区大小
    int socketSendBufSize(int fd, int size);

    // 设置socket内核接受缓冲区大小
    int socketRecvBufSize(int fd, int size);

    // 关闭socket描述符
    void closeSocket(int fd);

    // 获取sock的ip字符串
    std::string getIPFromSockaddr(const struct sockaddr *from);

    // 获取sock的ip字符串
    std::string getIPFromSockFd(int fd);

    // 检查自连接
    bool checkSelfConnect(int fd);
}

#endif // SPDNET_BASE_SOCKETAPI_H_