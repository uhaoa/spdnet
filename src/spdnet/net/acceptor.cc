#include <spdnet/net/acceptor.h>
#include <unordered_map>
#include <chrono>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <spdnet/net/socket_ops.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/event_service.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/platform.h>
#ifdef SPDNET_PLATFORM_LINUX
#include <spdnet/net/detail/impl_linux/epoll_channel.h>
#else
#include <spdnet/net/detail/impl_win/iocp_channel.h>
#endif


namespace spdnet {
    namespace net {
#ifdef SPDNET_PLATFORM_LINUX
		class TcpAcceptor::AcceptContext : public detail::Channel
		{
		public:
			friend class TcpAcceptor;
			AcceptContext(TcpAcceptor& acceptor)
				:acceptor_(acceptor) , 
				idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
			{

			}
			void trySend() override {}
			void onClose() override {}
			void tryRecv() override
			{
				sock_t accept_fd = ::accept(acceptor_.listen_fd_, nullptr, nullptr);
				if (accept_fd == -1) {
					if (current_errno() == EMFILE) {
						::close(idle_fd_);
						accept_fd = ::accept(acceptor_.listen_fd_, nullptr, nullptr);
						::close(accept_fd);
						idle_fd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
					}
					std::cerr << "accept error . errno:" << current_errno() << std::endl;
					return;
				}

				acceptor_.onAcceptSuccess(accept_fd);
			}

			~AcceptContext()
			{
				socket_ops::closeSocket(idle_fd_); 
			}
		private:
			TcpAcceptor& acceptor_;
			sock_t idle_fd_;
		};
#else
		class TcpAcceptor::AcceptContext : public detail::Channel
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

			AcceptContext(TcpAcceptor& acceptor)
				:acceptor_(acceptor)
			{
				/*asyncAccept(); */
			}

			~AcceptContext()
			{
				socket_ops::closeSocket(fd_);
			}

			void doComplete(size_t /*bytes_transferred*/ , std::error_code ec) override
			{
				if (!ec) {
					acceptor_.onAcceptSuccess(fd_);
					asyncAccept();
				}
				else {
					// ...
				}
			}

			CHAR* buffer() {
				return buffer_;
			}

			/*
			DWORD getAddressLength() {
				return sizeof(SOCKADDR_IN) + 16;
			}*/
		
			sock_t newSocket() {
				//socket_ = std::make_shared<TcpSocket>();
				fd_ = socket_ops::createSocket(acceptor_.addr_.family(), SOCK_STREAM, 0);	
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
					BOOL result = accept_ex(acceptor_.listen_fd_, newSocket(), buffer(),
						0, acceptor_.addr_.socket_addr_len(), acceptor_.addr_.socket_addr_len(), &bytes_read, this);
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
			CHAR	buffer_[(sizeof(SOCKADDR_IN) + 16) * 2];
			sock_t  fd_;
			std::atomic<void*> accept_ex_{ nullptr };
			TcpAcceptor& acceptor_;
		};

#endif

        TcpAcceptor::TcpAcceptor(EventService &service)
                : service_(service)
        {
			context_ = std::make_unique<AcceptContext>(*this);
        }

        void TcpAcceptor::start(const EndPoint &addr, TcpEnterCallback &&cb) {
			addr_ = addr;
			listen_fd_ = createListenSocket(addr);
            if (listen_fd_ == -1) {
                throw SpdnetException(std::string("listen error : ") + std::to_string(current_errno()));
            }
            enter_callback_ = std::move(cb);
			socket_ops::socketNonBlock(listen_fd_); 

            auto listen_thread = service_.getServiceThread(); 
			if (!listen_thread->getImpl()->startAccept(listen_fd_, context_.get())) {
				throw SpdnetException(std::string("listen error : ") + std::to_string(current_errno()));
			}
#if defined(SPDNET_PLATFORM_WINDOWS)
			context_->asyncAccept();
#endif
        }


        void TcpAcceptor::onAcceptSuccess(sock_t new_socket)
        {
			if (invalid_socket != new_socket) {
				this->service_.addTcpSession(new_socket, true , enter_callback_);
			}
        }

		sock_t TcpAcceptor::createListenSocket(const EndPoint &addr) {
			sock_t fd = ::socket(addr.family(), SOCK_STREAM, 0);
            if (fd == -1) {
                return -1;
            }

            int reuse_on = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse_on, sizeof(int)) < 0) {
                socket_ops::closeSocket(fd);
                return -1;
            }

            int ret = ::bind(fd, addr.socket_addr(), addr.socket_addr_len());
            if (ret == SPDNET_SOCKET_ERROR || listen(fd, 512) == -1) {
				socket_ops::closeSocket(fd);
                return -1;
            }

            return fd;

        }
    }
}