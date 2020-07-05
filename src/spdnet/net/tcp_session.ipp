#ifndef SPDNET_NET_TCP_SESSION_IPP_
#define SPDNET_NET_TCP_SESSION_IPP_

#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/service_thread.h>

namespace spdnet {
    namespace net {
        TcpSession::TcpSession(sock_t fd, bool is_server_side, std::shared_ptr<ServiceThread> service_thread)
                : service_thread_(service_thread) {
            socket_data_ = std::make_shared<SocketData>(fd, is_server_side);
        }

		TcpSession::~TcpSession() {
            
		}

        std::shared_ptr<TcpSession>
        TcpSession::create(sock_t fd, bool is_server_side, std::shared_ptr<ServiceThread> service_thread) {
            return std::make_shared<TcpSession>(fd, is_server_side, service_thread);
        }

        void TcpSession::setDisconnectCallback(TcpDisconnectCallback &&callback) {
            auto cb = std::move(callback);
            auto this_ptr = shared_from_this();
            socket_data_->setDisconnectCallback([cb, this_ptr]() mutable {
                if (cb) {
                    cb(this_ptr);
                }
                this_ptr = nullptr;
                /*this_ptr->loop_owner_->removeTcpSession(this_ptr->sock_fd());*/
            });
        }

        void TcpSession::send(const char *data, size_t len) {
            if (len <= 0)
                return;
            service_thread_->getImpl()->send(socket_data_, data, len);
        }

        void TcpSession::postShutDown() {
            auto this_ptr = shared_from_this();
            service_thread_->getExecutor()->post([this_ptr]() {
                this_ptr->service_thread_->getImpl()->shutdownSocket(this_ptr->socket_data_);
            });
        }

    }
}
#endif // SPDNET_NET_TCP_SESSION_IPP_