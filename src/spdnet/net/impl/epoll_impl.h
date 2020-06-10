#ifndef SPDNET_NET_EPOLL_IMPL_H_
#define SPDNET_NET_EPOLL_IMPL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/base.h>
#include <spdnet/base/current_thread.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/base/buffer_pool.h>

namespace spdnet::net::impl {
    using namespace base;

    class Channel;

    class WakeupChannel;

    class EpollImpl : public base::NonCopyable {
    public:
        using Ptr = std::shared_ptr<EpollImpl>;
    public:
        explicit EpollImpl(unsigned int) noexcept;

        virtual ~EpollImpl() noexcept;


        int epoll_fd() const {
            return epoll_fd_;
        }
		void runOnce();
		void send(Buffer* buffer , );
	private:
		bool linkEvent(int fd, const Channel* channel, uint32_t events);
	private:
        int epoll_fd_;
        int thread_id_;
		unsigned int wait_timeout_ms_;
		std::vector<epoll_event> event_entries_;
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

	class EpollImpl::DescriptorData : public base::NonCopyable
	{
	public:
	};
}
#endif  // SPDNET_NET_EPOLL_IMPL_H_