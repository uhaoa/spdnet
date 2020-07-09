#ifndef SPDNET_NET_IOCP_ACCEPT_CHANNEL_H_
#define SPDNET_NET_IOCP_ACCEPT_CHANNEL_H_
#include <iostream>
#include <spdnet/net/socket_ops.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>

namespace spdnet {
    namespace net {
		namespace detail {
			class IocpAcceptChannel : public Channel
			{
			public:
				typedef
					BOOL
					(PASCAL FAR* LPFN_ACCEPTEX)(
						_In_ SOCKET sListenSocket,
						_In_ SOCKET sAcceptSocket,
						_Out_writes_bytes_(dwReceiveDataLength + dwLocalAddressLength + dwRemoteAddressLength) PVOID lpOutputBuffer,
						_In_ DWORD dwReceiveDataLength,
						_In_ DWORD dwLocalAddressLength,
						_In_ DWORD dwRemoteAddressLength,
						_Out_ LPDWORD lpdwBytesReceived,
						_Inout_ LPOVERLAPPED lpOverlapped
						);

				IocpAcceptChannel(sock_t listen_fd , EndPoint addr, std::function<void(sock_t fd)>&& success_cb)
					:listen_fd_(listen_fd) , addr_(addr) , success_cb_(std::move(success_cb))
				{
				}

				~IocpAcceptChannel()
				{
					socket_ops::closeSocket(fd_);
				}

				void doComplete(size_t /*bytes_transferred*/, std::error_code ec) override
				{
					if (!ec) {
						success_cb_(fd_); 
						asyncAccept();
					}
					else {
						// ...
					}
				}

				CHAR* buffer() {
					return buffer_;
				}

				sock_t newSocket() {
					fd_ = socket_ops::createSocket(addr_.family(), SOCK_STREAM, 0);
					return fd_;
				}

				void asyncAccept()
				{
					if (!accept_ex_)
					{
						DWORD bytes = 0;
						GUID guid = WSAID_ACCEPTEX; //{ 0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92} };
						void* ptr = nullptr;
						if (::WSAIoctl(newSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER,
							&guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0)
						{
							// ...
						}
						if (fd_ != invalid_socket)
							socket_ops::closeSocket(fd_);
						accept_ex_ = ptr;
					}
					if (accept_ex_) {
						LPFN_ACCEPTEX accept_ex = (LPFN_ACCEPTEX)accept_ex_.load();
						reset();
						DWORD bytes_read = 0;
						BOOL result = accept_ex(listen_fd_, newSocket(), buffer(),
							0, addr_.socket_addr_len(), addr_.socket_addr_len(), &bytes_read, this);
						DWORD last_error = current_errno();
						if (!result && last_error != WSA_IO_PENDING) {
							// error
							std::cout << "::AcceptEx error ! errno:" << last_error << std::endl;
						}
					}
					else {
						// ...
					}
				}
			private:
				CHAR	buffer_[(sizeof(SOCKADDR_IN6) + 16) * 2];
				sock_t  fd_{ invalid_socket };
				std::atomic<void*> accept_ex_{ nullptr };
				sock_t listen_fd_{ invalid_socket };
				EndPoint addr_;
				std::function<void(sock_t fd)> success_cb_;
			};

			using AcceptChannelImpl = IocpAcceptChannel; 

		}
    }
}

#endif // SPDNET_NET_IOCP_ACCEPT_CHANNEL_H_