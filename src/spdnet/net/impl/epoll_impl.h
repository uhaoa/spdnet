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

namespace spdnet::net::impl {
    using namespace base;
    using namespace net;
    class Channel;
    class WakeupChannel;

    class EpollImpl : public base::NonCopyable {
    public:
		struct DescriptorData;
		using ImplDataType = DescriptorData;
		using ImplDataTypePtr = ImplDataType*; 
		using Ptr = EpollImpl*; 
        explicit EpollImpl(unsigned int) noexcept;

        virtual ~EpollImpl() noexcept;

        int epoll_fd() const {
            return epoll_fd_;
        }

        void onSessionEnter(TcpSession::Ptr session);
		void asyncConnect(ConnectSession::Ptr session);
		void runOnce();
		void send(TcpSession::Ptr session);
	private:
		bool linkEvent(int fd, const Channel* channel, uint32_t events);
		void flushBuffer(ImplDataTypePtr& session);
        void regWriteEvent(ImplDataTypePtr& session);
        void prepareClose(ImplDataTypePtr& session);
        void recv(ImplDataTypePtr& session);
	private:
        int epoll_fd_;
        int thread_id_;
		unsigned int wait_timeout_ms_;
		std::vector<epoll_event> event_entries_;
		EventLoopPtr loop_owner_;
		/*
        std::mutex task_mutex_;
        std::vector<AsynLoopTask> async_tasks;
        std::vector<AsynLoopTask> tmp_async_tasks;
        std::vector<AfterLoopTask> after_loop_tasks;
        std::vector<AfterLoopTask> tmp_after_loop_tasks;
        std::shared_ptr<std::thread> loop_thread_;
        unsigned int wait_timeout_ms_;
        std::vector<epoll_event> event_entries_;
        std::unique_ptr<WakeupChannel> wake_up_;
        std::unordered_map<int, TcpSession::Ptr> tcp_sessions_;
        BufferPool buffer_pool_;
		*/
    };

	class TcpSessionChannel; 
	struct EpollImpl::DescriptorData : public base::NonCopyable
	{
		int fd_;
		Buffer recv_buffer_;
		std::deque<Buffer*> send_buffer_list_;
		std::deque<Buffer*> pending_buffer_list_;
		SpinLock send_guard_;
		volatile bool has_closed_{ false };
		volatile bool is_post_flush_{ false };
		volatile bool is_can_write_{ true };
		TcpSessionChannel* channel_{nullptr}
	};


}
#endif  // SPDNET_NET_EPOLL_IMPL_H_