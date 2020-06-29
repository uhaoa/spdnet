#ifndef SPDNET_NET_IOCP_IMPL_H_
#define SPDNET_NET_IOCP_IMPL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/base/buffer.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/end_point.h>

namespace spdnet {
	namespace net {
		class EventLoop;
		//ServiceThread
		class AsyncConnector;
		namespace detail {
			class SocketRecieveChannel;
			class SocketWakeupChannel;
			class Channel;
			class SocketImplData : public SocketDataBase {
			public:
				friend class IocpImpl;
				friend class SocketSendChannel;
				friend class SocketRecieveChannel;

				using Ptr = std::shared_ptr<SocketImplData>;

				SocketImplData(std::shared_ptr<EventLoop> loop, sock_t fd , bool is_server_side);
			private:
				std::shared_ptr<SocketRecieveChannel> recv_channel_;
				std::shared_ptr<SocketSendChannel> send_channel_;
			};

			class SocketWakeupOp;
			class IocpImpl : public spdnet::base::NonCopyable {
			public:
				friend class SocketRecieveChannel;
				friend class SocketSendChannel;

				explicit IocpImpl(EventLoop& loop) noexcept;

				virtual ~IocpImpl() noexcept;

				bool onSocketEnter(SocketImplData& socket_data);

				void send(SocketImplData& socket_data, const char* data, size_t len);

				void flushBuffer(SocketImplData& socket_data);

				void runOnce(uint32_t timeout);

				void wakeup();

				bool startAccept(sock_t listen_fd, Channel* op);

				bool asyncConnect(sock_t fd , const EndPoint& addr , Channel* op);

				void shutdownSocket(SocketImplData& socket_data) {}
			private:
				void startRecv(SocketImplData& socket_data);

				void closeSocket(SocketImplData& socket_data);
			private:
				HANDLE  handle_;
				EventLoop& loop_ref_;
				std::shared_ptr<SocketWakeupChannel> wakeup_op_;
				std::atomic<void*> connect_ex_{nullptr};
				std::vector<std::shared_ptr<Channel>> del_channel_list_;
			};
		}
	}
}
#endif  // SPDNET_NET_IOCP_IMPL_H_