#include <spdnet/net/acceptor.h>
#include <unordered_map>
#include <chrono>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <spdnet/base/socket_api.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/event_service.h>
#include <spdnet/net/end_point.h>
#ifdef SPDNET_PLATFORM_LINUX
#include <spdnet/net/detail/impl_linux/channel.h>
#else
#include <spdnet/net/detail/impl_win/iocp_operation.h>
#endif

namespace spdnet {
    namespace net {
#ifdef SPDNET_PLATFORM_LINUX
		class TcpAcceptor::AcceptContext : public detail::Channel
		{
		public:
			friend class TcpAcceptor;
			AcceptContext()
			{

			}
			void trySend() override {}
			void onClose() override {}
			void tryRecv() override
			{

			}

		private:
			TcpAcceptor& acceptor_;
		};
#else
		class TcpAcceptor::AcceptContext : public Operation
		{
		public:
			AcceptContext(TcpAcceptor& acceptor , sock listen_fd)
				:acceptor_(acceptor) listen_fd_(listen_fd)
			{
				asyncAccept(); 
			}

		private:
			void doComplete(size_t /*bytes_transferred*/) override
			{
				/*
				LPSOCKADDR local_addr = 0;
				int local_addr_length = 0;
				LPSOCKADDR remote_addr = 0;
				int remote_addr_length = 0;
				GetAcceptExSockaddrs(buffer, 0, getAddressLength(),
					getAddressLength(), &local_addr, &local_addr_length,
					&remote_addr, &remote_addr_length);
					*/
				acceptor_.onAcceptSuccess(socket_);
				asyncAccept();
			}

			CHAR* buffer() {
				return buffer_;
			}

			DWORD getAddressLength() {
				return sizeof(SOCKADDR_IN) + 16;
			}
		
			sock makeNewSocket() {
				return socket_->sock_fd();
			}

			void asyncAccept() 
			{
				DWORD bytes_read = 0;
				BOOL result = ::AcceptEx(listen_fd_, makeNewSocket(), buffer(),
					0, getAddressLength(), getAddressLength(), &bytes_read, this);
				DWORD last_error = current_errno();
				if (!result && last_error != WSA_IO_PENDING) {
					// error
					std::cout << "::AcceptEx error ! errno:" << last_error << std::endl; .
				}
			}
		private:
			CHAR		buffer_[(sizeof(SOCKADDR_IN) + 16) * 2];
			std::shared_ptr<TcpSocket> socket_;
			sock listen_fd_; 
			TcpAcceptor& acceptor_;
		};

#endif

        TcpAcceptor::TcpAcceptor(EventService &service)
                : service_(service)
        {
			context_ = std::make_unique<AcceptContext>(*this);
        }

        void TcpAcceptor::start(const EndPoint &addr, TcpSession::TcpEnterCallback &&cb) {
            const sock listen_fd = createListenSocket(addr);
            if (listen_fd == -1) {
                throw SpdnetException(std::string("listen error : ") + std::to_string(current_errno()));
            }
            enter_callback_ = std::move(cb);
            listen_socket_ = std::make_shared<ListenSocket>(listen_fd);
            listen_socket_->setNonblock();

            auto listen_loop = service_.getEventLoop(); 
            listen_loop->getImpl().startAccept(listen_socket_->sock_fd(),context_.get());

            /*
            run_listen_ = std::make_shared<bool>(true);
            listen_thread_ = std::make_shared<std::thread>(
                    [listen_socket, enter_callback = std::move(cb), this]() mutable {
                        while (*(this->run_listen_)) {
							auto tcp_socket = listen_socket->accept();
							if (nullptr != tcp_socket) {
								this->service_.addTcpSession(std::move(tcp_socket), enter_callback);
							}
                        }
                    }
            );
            */
        }


        void TcpAcceptor::onAcceptSuccess(std::shared_ptr<TcpSocket> new_socket)
        {
			if (nullptr != new_socket) {
				this->service_.addTcpSession(std::move(new_socket), enter_callback_);
			}
        }

        sock TcpAcceptor::createListenSocket(const EndPoint &addr) {
            sock fd = ::socket(addr.family(), SOCK_STREAM, 0);
            if (fd == -1) {
                return -1;
            }

            int reuse_on = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse_on, sizeof(int)) < 0) {
                spdnet::base::closeSocket(fd);
                return -1;
            }

            int ret = ::bind(fd, addr.socket_addr(), addr.socket_addr_len());
            if (ret == -1 || listen(fd, 512) == -1) {
                spdnet::base::closeSocket(fd);
                return -1;
            }

            return fd;

        }
    }
}