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

namespace spdnet {
    namespace net {
        TcpAcceptor::TcpAcceptor(EventService &service)
                : service_(service)
        {

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
            listen_loop->getImpl().startAccept(listen_fd);

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

        void TcpAcceptor::doAccept()
        {
			auto tcp_socket = listen_socket_->accept();
			if (nullptr != tcp_socket) {
				this->service_.addTcpSession(std::move(tcp_socket), enter_callback_);
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