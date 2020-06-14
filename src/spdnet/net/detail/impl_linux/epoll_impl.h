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
        class TcpSessionChannel ;

        struct SocketImplData : public base::NonCopyable {
            SocketImplData(TcpSession::Ptr , EventLoop&) noexcept ;

            Buffer recv_buffer_;
            std::deque<Buffer *> send_buffer_list_;
            std::deque<Buffer *> pending_buffer_list_;
            base::SpinLock send_guard_;
            volatile bool has_closed_{false};
            volatile bool is_post_flush_{false};
            volatile bool is_can_write_{true};
            std::shared_ptr<TcpSessionChannel> channel_;
        };

        class EpollImpl : public base::NonCopyable {
        public:
            friend class TcpSessionChannel;
            explicit EpollImpl(EventLoop &loop_owner) noexcept;

            virtual ~EpollImpl() noexcept;

            void onTcpSessionEnter(TcpSession::Ptr session);

            void runOnce(uint32_t timeout);

            void send(TcpSession::Ptr session);

            void wakeup();

            void closeSession(TcpSession::Ptr session);

            void shutdownSession(TcpSession::Ptr session);

            int epoll_fd() const { return epoll_fd_; }

            bool linkChannel(int fd, const Channel *channel, uint32_t events);
        private:
            void addWriteEvent(TcpSession::Ptr session);

            void cancelWriteEvent(TcpSession::Ptr session);

            void flushBuffer(TcpSession::Ptr session);

            void doRecv(TcpSession::Ptr session);

        private:
            int epoll_fd_;
            std::unique_ptr<WakeupChannel> wake_up_;
            std::vector<epoll_event> event_entries_;
            EventLoop &loop_owner_ref_;
            std::vector<std::function<void()>> delay_tasks;
        };
    }
}
#endif  // SPDNET_NET_EPOLL_IMPL_H_