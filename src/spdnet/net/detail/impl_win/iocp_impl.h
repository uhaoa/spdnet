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
			class SocketRecieveOp;
			class SocketWakeupOp;
			class Operation;
			class SocketImplData : public SocketDataBase {
			public:
				friend class IocpImpl;
				friend class SocketSendOp;
				friend class SocketRecieveOp;

				using Ptr = std::shared_ptr<SocketImplData>;

				SocketImplData(std::shared_ptr<EventLoop> loop, sock_t fd , bool is_server_side);
			private:
				std::shared_ptr<SocketRecieveOp> recv_op_;
				std::shared_ptr<SocketSendOp> send_op_;
			};

			class SocketWakeupOp;
			class IocpImpl : public spdnet::base::NonCopyable {
			public:
				friend class SocketRecieveOp; 
				friend class SocketSendOp; 

				explicit IocpImpl(EventLoop& loop) noexcept;

				virtual ~IocpImpl() noexcept;

				bool onSocketEnter(SocketImplData& socket_data);

				void send(SocketImplData& socket_data, const char* data, size_t len);

				void flushBuffer(SocketImplData& socket_data);

				void runOnce(uint32_t timeout);

				void wakeup();

				bool startAccept(sock_t listen_fd, Operation* op);

				bool asyncConnect(sock_t fd , const EndPoint& addr , Operation* op);

				void shutdownSocket(SocketImplData& socket_data) {}
			private:
				void startRecv(SocketImplData& socket_data);

				void closeSocket(SocketImplData& socket_data);
			private:
				HANDLE  handle_;
				EventLoop& loop_ref_;
				std::shared_ptr<SocketWakeupOp> wakeup_op_;
				std::atomic<void*> connect_ex_{nullptr};
				std::vector<std::shared_ptr<Operation>> del_operation_list_;
			};
		}
	}
}
#endif  // SPDNET_NET_IOCP_IMPL_H_