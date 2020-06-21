#ifndef SPDNET_NET_SOCKET_DATA_H_
#define SPDNET_NET_SOCKET_DATA_H_

#include <memory>
#include <spdnet/base/platform.h>
#include <spdnet/base/noncopyable.h>

namespace spdnet {
    namespace net {
		class SocketDataBase : public base::NonCopyable {
		public:
			using TcpDataCallback = std::function<size_t(const char*, size_t len)>;
			using TcpDisconnectCallback = std::function<void()>;

			SocketDataBase(std::shared_ptr<TcpSocket> socket)
				:socket_(socket)
			{

			}

			void setDisconnectCallback(TcpDisconnectCallback&& callback) {
				disconnect_callback_ = callback;
			}
			void setDataCallback(TcpDataCallback&& callback) {
				data_callback_ = callback;
			}
			void setNodelay() {
				socket_->setNoDelay();
			}
			sock sock_fd() const {
				return socket_->sock_fd();
			}
			void setMaxRecvBufferSize(size_t max_size) {
				max_recv_buffer_size_ = max_size;
			}
		protected:
			std::shared_ptr<TcpSocket> socket_;
			TcpDisconnectCallback disconnect_callback_;
			TcpDataCallback data_callback_;
			base::Buffer recv_buffer_;
			size_t max_recv_buffer_size_ = 64 * 1024;
			std::deque<base::Buffer*> send_buffer_list_;
			std::deque<base::Buffer*> pending_buffer_list_;
			spdnet::base::SpinLock send_guard_;
			volatile bool has_closed_{ false };
			volatile bool is_post_flush_{ false };
			/*
			std::shared_ptr<SocketRecieveOp> recv_op_;
			std::shared_ptr<SocketSendOp> send_op_;
			*/
		};
    }
}

#endif  // SPDNET_NET_SOCKET_DATA_H_