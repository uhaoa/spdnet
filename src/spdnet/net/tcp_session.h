#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/base/platform.h>
#if defined SPDNET_PLATFORM_LINUX
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#else
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#endif

namespace spdnet {
    namespace net {
        class EventLoop;
		class SPDNET_EXPORT TcpSession : public spdnet::base::NonCopyable, public std::enable_shared_from_this<TcpSession> {
		public:
			friend class EventLoop;
			friend class EventService;
			using Ptr = std::shared_ptr<TcpSession>;
			using TcpDataCallback = std::function<size_t(const char*, size_t len)>;
			using TcpDisconnectCallback = std::function<void(Ptr)>;
		public:
			inline TcpSession(sock_t fd, bool is_server_side, std::shared_ptr<IoObjectImplType> impl , std::shared_ptr<TaskExecutor> task_executor);

			inline ~TcpSession() = default;

			inline void postShutDown();

			inline void setDisconnectCallback(TcpDisconnectCallback&& callback);

			inline void setDataCallback(TcpDataCallback&& callback) {
				socket_data_->setDataCallback(std::move(callback));
			}

			inline void setMaxRecvBufferSize(size_t len) {
				socket_data_->setMaxRecvBufferSize(len);
			}

			inline void setNodelay() {
				socket_data_->setNodelay();
			}

			inline void send(const char* data, size_t len);


			inline sock_t sock_fd() const {
				return socket_data_->sock_fd();
			}

		public:
			inline static Ptr create(sock_t fd, bool is_server_side, std::shared_ptr<IoObjectImplType> loop, std::shared_ptr<TaskExecutor> task_executor);
		private:
			std::shared_ptr<TcpSocketData> socket_data_;
			std::shared_ptr<IoObjectImplType> io_impl_;
			std::shared_ptr<TaskExecutor> task_executor_; 
		};

    }
}

#include <spdnet/net/tcp_session.ipp>

#endif // SPDNET_NET_TCP_SESSION_H_