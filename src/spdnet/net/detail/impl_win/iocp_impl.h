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

namespace spdnet::net {
    class EventLoop;
	//ServiceThread
    class AsyncConnector;
    namespace detail {

		class SocketImplData : public SocketDataBase {
		public:
			friend class IocpImpl;
			friend class SocketSendOp;
			friend class SocketRecieveOp;
			class SocketRecieveOp;
			class SocketWakeupOp;
			using Ptr = std::shared_ptr<SocketImplData>;

			SocketImplData(std::shared_ptr<EventLoop> loop, std::shared_ptr<TcpSocket> socket);
		private:
			std::shared_ptr<SocketRecieveOp> recv_op_;
			std::shared_ptr<SocketSendOp> send_op_;
		};

        class IocpImpl : public base::NonCopyable {
        public:
			class SocketSendOp;

            explicit IocpImpl(EventLoop &loop) noexcept;

            virtual ~IocpImpl() noexcept;

			bool onSocketEnter(SocketImplData& socket_data);

			void send(SocketImplData& socket_data, const char* data, size_t len); 

			void flushBuffer(SocketImplData& socket_data); 

            void runOnce(uint32_t timeout);

			void wakeup();
        private:
			void startRecv(SocketImplData& socket_data);
        private:
			HANDLE  handle_;
            EventLoop &loop_ref_;
			std::shared_ptr<SocketWakeupOp> wakeup_op_;
        };
    }
}
#endif  // SPDNET_NET_IOCP_IMPL_H_