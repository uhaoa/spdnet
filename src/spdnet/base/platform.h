#ifndef SPDNET_BASE_PLATFORM_H
#define SPDNET_BASE_PLATFORM_H

#if defined _MSC_VER || defined __MINGW32__
#define SPDNET_PLATFORM_WINDOWS 1
#else
#define SPDNET_PLATFORM_LINUX 1
#endif

#if defined(SPDNET_PLATFORM_WINDOWS)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif


#ifdef __GNUC__
#define SPDNET_PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))
#else
#define SPDNET_PREDICT_TRUE(x) (x)
#endif


#ifdef __GNUC__
#define SPDNET_PREDICT_FALSE(x) (__builtin_expect(x, 0))
#else
#define SPDNET_PREDICT_FALSE(x) (x)
#endif


#ifdef SPDNET_PLATFORM_LINUX

#include<netdb.h>
#include<cerrno>
#include<fcntl.h>
#include<unistd.h>
#include<csignal>
#include<sys/uio.h>
#include<sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

using sock_t = int;

constexpr sock_t invalid_socket = -1;

inline int current_errno() {
	return errno;
}

#define SPDNET_SOCKET_ERROR -1

#define SPDNET_EXPORT 
#else
#include <winsock2.h>
#include <mswsock.h>
#include <WinError.h>
//#include <winsock.h>
#include <Ws2tcpip.h>
#include <errno.h>

using sock_t = SOCKET;

constexpr sock_t invalid_socket = INVALID_SOCKET;

inline int current_errno() {
	return WSAGetLastError();
}

#define SPDNET_SOCKET_ERROR SOCKET_ERROR

#define SPDNET_EXPORT  __declspec(dllexport)
#endif


#endif  // SPDNET_BASE_PLATFORM_H