#ifndef SPDNET_NET_SERVICE_THREAD_H_
#define SPDNET_NET_SERVICE_THREAD_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/current_thread.h>
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/task_executor.h>
#ifdef SPDNET_PLATFORM_LINUX
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#else
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#endif

namespace spdnet {
    namespace net {
		class TcpSession; 
		using TcpEnterCallback = std::function<void(std::shared_ptr<TcpSession>)>;
		
        class SPDNET_EXPORT ServiceThread : public spdnet::base::NonCopyable {
        public:
            inline explicit ServiceThread(unsigned int);

			inline ~ServiceThread() = default;

			inline void run(std::shared_ptr<bool>);

			/*
            inline bool isInLoopThread() const {
                return thread_id_ == current_thread::tid();
            }*/

			inline void post(AsynTaskFunctor&& task);

			inline void onTcpSessionEnter(sock_t fd, std::shared_ptr<TcpSession> tcp_session, const TcpEnterCallback& enter_callback);

			inline std::shared_ptr<TcpSession> getTcpSession(sock_t fd);

			inline void removeTcpSession(sock_t fd);

			inline void addTcpSession(sock_t fd , std::shared_ptr<TcpSession>);

			inline const std::shared_ptr<std::thread>& getThread() const {
                return thread_;
            }

			inline std::shared_ptr<detail::IoObjectImplType> getImpl() {
				return io_impl_;
			}

			std::shared_ptr<TaskExecutor> getExecutor() {
				return task_executor_; 
			}

			thread_id_t thread_id() const { return thread_id_; }
        private:
            thread_id_t thread_id_;
            std::shared_ptr<detail::IoObjectImplType> io_impl_;
			std::shared_ptr<TaskExecutor> task_executor_;
            std::shared_ptr<std::thread> thread_;
            unsigned int wait_timeout_ms_;
			std::unordered_map<sock_t, std::shared_ptr<TcpSession>> tcp_sessions_;
        };
    }
}

#include <spdnet/net/service_thread.ipp>

#endif  // SPDNET_NET_SERVICE_THREAD_H_