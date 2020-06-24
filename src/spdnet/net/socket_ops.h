#ifndef SPDNET_BASE_SOCKETAPI_H_
#define SPDNET_BASE_SOCKETAPI_H_

#include <spdnet/base/platform.h>
#include <string>

namespace spdnet::net::socket_ops {
    int socketNoDelay(sock_t fd);

    bool socketBlock(sock_t fd);

    bool socketNonBlock(sock_t fd);

    int socketSendBufSize(sock_t fd, int size);

    int socketRecvBufSize(sock_t fd, int size);

    void closeSocket(sock_t fd);

    std::string getIPFromSockaddr(const struct sockaddr *from);

    std::string getIPFromSockFd(sock_t fd);

    bool checkSelfConnect(sock_t fd);

	inline void clearErrno()
	{
#if defined(SPDNET_PLATFORM_WINDOWS)
		WSASetLastError(0);
#else
		errno = 0;
#endif
	}

	inline sock_t createSocket(int family, int type, int protocol)
	{
#if defined(SPDNET_PLATFORM_WINDOWS)
		sock_t s = ::WSASocketW(af, type, protocol, 0, 0,
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

#endif // SPDNET_BASE_SOCKETAPI_H_