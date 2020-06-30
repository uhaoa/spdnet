#ifndef SPDNET_NET_TASK_EXECUTOR_H
#define SPDNET_NET_TASK_EXECUTOR_H

#include <iostream>
#include <spdnet/base/platform.h>
#include <spdnet/net/io_type_define.h>
#include <spdnet/net/current_thread.h>
#if defined(SPDNET_PLATFORM_LINUX)
#include <spdnet/net/detail/impl_win/epoll_wakeup_channel.h>
#else
#include <spdnet/net/detail/impl_win/iocp_wakeup_channel.h>
#endif

namespace spdnet {
    namespace net {
		using AsynTaskFunctor = std::function<void()>;
#if defined(SPDNET_PLATFORM_LINUX)
		using WakeupChannel = detail::EpollWakeupChannel; 
#else
		using WakeupChannel = detail::IocpWakeupChannel;
#endif
        class TaskExecutor : public spdnet::base::NonCopyable {
        public:
			void post(AsynTaskFunctor&& task) {
				if (current_thread::tid() == thread_id_) {
					// immediate exec
					task(); 
				}
				else {
					{
						std::lock_guard<std::mutex> lck(task_mutex_);
						async_tasks.emplace_back(std::move(task));
					}
					channel_->wakeup(); 
				}
			}
			void run()
			{
				{
					std::lock_guard<std::mutex> lck(task_mutex_);
					tmp_async_tasks.swap(async_tasks);
				}
				for (auto& task : tmp_async_tasks) {
					task();
				}
				tmp_async_tasks.clear();
			}
			
			void setThreadId(thread_id_t id) {
				thread_id_ = id; 
			}

			void setWakeupChannel(std::shared_ptr<WakeupChannel> channel) {
				channel_ = channel; 
			}
		private:
			thread_id_t thread_id_; 
			std::mutex task_mutex_;
			std::vector<AsynTaskFunctor> async_tasks;
			std::vector<AsynTaskFunctor> tmp_async_tasks;
			std::shared_ptr<WakeupChannel> channel_;
        };
    }
}
#endif // SPDNET_NET_TASK_EXECUTOR_H
