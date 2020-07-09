#ifndef SPDNET_NET_ACCEPTOR_IPP_
#define SPDNET_NET_ACCEPTOR_IPP_

#include <spdnet/net/acceptor.h>
#include <iostream>
#include <algorithm>
#include <spdnet/net/socket_ops.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/event_service.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/platform.h>

namespace spdnet {
    namespace net {

        TcpAcceptor::TcpAcceptor(EventService &service)
                : service_(service) {
        }

		TcpAcceptor::~TcpAcceptor()
		{
			auto collector = listen_thread_->getChannelCollector();
			assert(collector); 
			collector->putChannel(accept_channel_); 
		}

        void TcpAcceptor::start(const EndPoint &addr, TcpEnterCallback && enter_cb) {
            addr_ = addr;
            listen_fd_ = createListenSocket(addr);
            if (listen_fd_ == invalid_socket) {
                throw SpdnetException(std::string("listen error : ") + std::to_string(current_errno()));
            }
            socket_ops::socketNonBlock(listen_fd_);
			auto& service = service_;
#if defined(SPDNET_PLATFORM_WINDOWS)
			accept_channel_ = std::make_shared<detail::AcceptChannelImpl>(listen_fd_, addr, [&service, enter_cb](sock_t new_socket) {
				service.addTcpSession(new_socket, true, enter_cb);

				});
#else
			accept_channel_ = std::make_shared<detail::AcceptChannelImpl>(listen_fd_, [&service, enter_cb](sock_t new_socket) {
				service.addTcpSession(new_socket, true, enter_cb);
				});
#endif

			listen_thread_ = service_.getServiceThread();
            if (!listen_thread_->getImpl()->startAccept(listen_fd_, accept_channel_.get())) {
                throw SpdnetException(std::string("listen error : ") + std::to_string(current_errno()));
            }
        }

        sock_t TcpAcceptor::createListenSocket(const EndPoint &addr) {
            sock_t fd = ::socket(addr.family(), SOCK_STREAM, 0);
            if (fd == invalid_socket) {
                return invalid_socket;
            }

            int reuse_on = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse_on, sizeof(int)) < 0) {
                socket_ops::closeSocket(fd);
                return invalid_socket;
            }

            int ret = ::bind(fd, addr.socket_addr(), addr.socket_addr_len());
            if (ret == SPDNET_SOCKET_ERROR || listen(fd, 512) == SPDNET_SOCKET_ERROR) {
                socket_ops::closeSocket(fd);
                return invalid_socket;
            }

            return fd;

        }
    }
}

#endif // SPDNET_NET_ACCEPTOR_IPP_