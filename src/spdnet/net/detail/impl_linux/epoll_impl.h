#ifndef SPDNET_NET_EPOLL_IMPL_H_
#define SPDNET_NET_EPOLL_IMPL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet::net {
    class EventLoop;

    class AsyncConnector;
    namespace detail {
        class Channel;

        class WakeupChannel;
        class TcpSessionChannel;

        class EpollImpl : public base::NonCopyable {
        public:
            friend class TcpSessionChannel;

			class SocketImplData : public base::NonCopyable {
			public:
				friend class EpollImpl;
				using Ptr = std::shared_ptr<SocketImplData>;
				using TcpDataCallback = std::function<size_t(const char*, size_t len)>;
				using TcpDisconnectCallback = std::function<void(Ptr)>;

				SocketImplData(EpollImpl& impl , std::shared_ptr<TcpSocket> socket)
					:socket_(std::move(socket))
				{
					channel_ = std::make_shared<TcpSessionChannel>(impl, *this);
				}
				void setDisconnectCallback(TcpDisconnectCallback&& callback) {
					disconnect_callback_ = callback;
				}
				void setDataCallback(TcpDataCallback&& callback){
					data_callback_ = callback;
				}
				void setNodelay() {
					socket_->setNoDelay();
				}
			private:
				std::shared_ptr<TcpSocket> socket_;
				TcpDisconnectCallback disconnect_callback_;
				TcpDataCallback data_callback_;
				Buffer recv_buffer_;
				std::deque<Buffer*> send_buffer_list_;
				std::deque<Buffer*> pending_buffer_list_;
				spdnet::base::SpinLock send_guard_;
				volatile bool has_closed_{ false };
				volatile bool is_post_flush_{ false };
				volatile bool is_can_write_{ true };
				std::shared_ptr<TcpSessionChannel> channel_;
			};



            explicit EpollImpl(EventLoop &loop_owner) noexcept;

            virtual ~EpollImpl() noexcept;

			void onTcpSessionEnter(SocketImplData& socket_data);

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
            std::vector<std::function<void()>> delay_tasks;
        };
    }
}
#endif  // SPDNET_NET_EPOLL_IMPL_H_