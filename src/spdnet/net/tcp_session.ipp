#ifndef SPDNET_NET_TCP_SESSION_IPP_
#define SPDNET_NET_TCP_SESSION_IPP_

#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>

namespace spdnet {
    namespace net {
        TcpSession::TcpSession(sock_t fd, bool is_server_side , std::shared_ptr<IoObjectImplType> impl , std::shared_ptr<TaskExecutor> task_executor)
                :io_impl_(impl), task_executor_(task_executor){
			socket_data_ = std::make_shared<TcpSocketData>(fd , is_server_side);
        }

        TcpSession::Ptr TcpSession::create(sock_t fd, bool is_server_side , std::shared_ptr<IoObjectImplType> impl, std::shared_ptr<TaskExecutor> task_executor) {
            return std::make_shared<TcpSession>(fd, is_server_side ,  impl , task_executor);
        }

		void TcpSession::setDisconnectCallback(TcpDisconnectCallback&& callback) {
			auto cb = std::move(callback);
			auto this_ptr = shared_from_this();
			socket_data_->setDisconnectCallback([cb, this_ptr]() {
				if (cb) {
					cb(this_ptr);
				}
				/*this_ptr->loop_owner_->removeTcpSession(this_ptr->sock_fd());*/
			});
		}

        void TcpSession::send(const char *data, size_t len) {
            if (len <= 0)
                return;
			io_impl_->send(*socket_data_ , data , len);
        }

        void TcpSession::postShutDown() {
            auto io_impl = io_impl_;
            auto this_ptr = shared_from_this();
			task_executor_->post([io_impl, this_ptr]() {
				io_impl->shutdownSocket(*this_ptr->socket_data_);
            });
        }

    }
}
#endif // SPDNET_NET_TCP_SESSION_IPP_