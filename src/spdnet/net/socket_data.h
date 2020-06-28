#ifndef SPDNET_NET_SOCKET_DATA_H_
#define SPDNET_NET_SOCKET_DATA_H_

#include <memory>
#include <spdnet/base/platform.h>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/net/socket_ops.h>

namespace spdnet {
    namespace net {
		class SocketDataBase : public base::NonCopyable {
		public:
			using TcpDataCallback = std::function<size_t(const char*, size_t len)>;
			using TcpDisconnectCallback = std::function<void()>;

			SocketDataBase(sock_t fd , bool is_server_side)
				:fd_(fd), is_server_side_(is_server_side)
			{

			}

			void setDisconnectCallback(TcpDisconnectCallback&& callback) {
				disconnect_callback_ = callback;
			}
			void setDataCallback(TcpDataCallback&& callback) {
				data_callback_ = callback;
			}
			void setNodelay() {
				socket_ops::socketNoDelay(fd_); 
			}
			sock_t sock_fd() const {
				return fd_;
			}
			void setMaxRecvBufferSize(size_t max_size) {
				max_recv_buffer_size_ = max_size;
			}
			bool isServerSide() { return is_server_side_; }
		protected:
			sock_t fd_;
			bool is_server_side_{ false }; 
			TcpDisconnectCallback disconnect_callback_;
			TcpDataCallback data_callback_;
			spdnet::base::Buffer recv_buffer_;
			size_t max_recv_buffer_size_ = 64 * 1024;
			std::deque<spdnet::base::Buffer*> send_buffer_list_;
			std::deque<spdnet::base::Buffer*> pending_buffer_list_;
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