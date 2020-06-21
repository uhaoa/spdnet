#ifndef SPDNET_NET_EPOLL_IMPL_H_
#define SPDNET_NET_EPOLL_IMPL_H_

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

    class AsyncConnector;
    namespace detail {
        class Channel;
        class WakeupChannel;
        class TcpSessionChannel;

		class SocketImplData : public SocketDataBase {
		public:
			friend class EpollImpl;
			using Ptr = std::shared_ptr<SocketImplData>;

			SocketImplData(std::shared_ptr<EventLoop> loop, std::shared_ptr<TcpSocket> socket);
		private:

			volatile bool is_can_write_{ true };
			std::shared_ptr<TcpSessionChannel> channel_;
		};


        class EpollImpl : public base::NonCopyable {
        public:
            friend class TcpSessionChannel;

            explicit EpollImpl(EventLoop &loop_owner) noexcept;

            virtual ~EpollImpl() noexcept;

			bool onSocketEnter(SocketImplData& socket_data);

            void runOnce(uint32_t timeout);

            void send(SocketImplData& socket_data , const char*  data ,size_t len);

            void wakeup(); 

            void shutdownSocket(SocketImplData& socket_data);

            int epoll_fd() const { return epoll_fd_; }

            bool linkChannel(int fd, const Channel *channel, uint32_t events);

        private:
			void closeSocket(SocketImplData& socket_data);

            void addWriteEvent(SocketImplData & socket_data);

            void cancelWriteEvent(SocketImplData & socket_data);

            void flushBuffer(SocketImplData & socket_data);

            void doRecv(SocketImplData &socket_data);

        private:
            int epoll_fd_;
            std::unique_ptr<WakeupChannel> wake_up_;
            std::vector<epoll_event> event_entries_;
            EventLoop &loop_ref_;
        };
    }
}
#endif  // SPDNET_NET_EPOLL_IMPL_H_