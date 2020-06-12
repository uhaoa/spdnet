#ifndef SPDNET_NET_EPOLL_IMPL_H_
#define SPDNET_NET_EPOLL_IMPL_H_
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/base.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet::net {
    using namespace base;
    using namespace net;

    class EpollImpl : public base::NonCopyable {
    public:
		struct IoPrivateData;
		class WakeupChannel; 
		class Channel; 

		friend class TcpSessionChannel;

        explicit EpollImpl(EventLoop& loop_owner) noexcept;
        virtual ~EpollImpl() noexcept;

        void onTcpSessionEnter(TcpSession::Ptr session);
		void asyncConnect(ConnectSession::Ptr session);
		void runOnce(uint32_t timeout);
		void send(TcpSession::Ptr session);
        void recycle_private_data(void* ptr);
		void wakeup() {
			wake_up_->wakeup(); 
		}
		void closeSession(TcpSession::Ptr session);
		void shutdownSession(TcpSession::Ptr session);
	private:
		void addWriteEvent(TcpSession::Ptr session); 
		void cancelWriteEvent(TcpSession::Ptr session);
		bool linkEvent(int fd, const Channel* channel, uint32_t events);
		void flushBuffer(TcpSession::Ptr session);
        void doRecv(TcpSession::Ptr session);
		
	private:
        int epoll_fd_;
        int thread_id_;
		std::unique_ptr<WakeupChannel> wake_up_; 
		std::vector<epoll_event> event_entries_;
		EventLoop& loop_owner_;
    };
}
#endif  // SPDNET_NET_EPOLL_IMPL_H_