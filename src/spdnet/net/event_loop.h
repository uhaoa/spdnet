#ifndef SPDNET_NET_EVENT_LOOP_H_
#define SPDNET_NET_EVENT_LOOP_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/current_thread.h>
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/io_type_define.h>
#include <spdnet/net/task_executor.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
    namespace net {
		using TcpEnterCallback = std::function<void(std::shared_ptr<TcpSession>)>;

        class SPDNET_EXPORT EventLoop : public spdnet::base::NonCopyable {
        public:
            inline explicit EventLoop(unsigned int);

			inline ~EventLoop() = default;

			inline void run(std::shared_ptr<bool>);

            inline bool isInLoopThread() const {
                return thread_id_ == current_thread::tid();
            }

			inline void post(AsynTaskFunctor&& task);

			inline void onTcpSessionEnter(std::shared_ptr<TcpSession> tcp_session, const TcpEnterCallback& enter_callback);

			inline std::shared_ptr<TcpSession> getTcpSession(sock_t fd);

			inline void addTcpSession(std::shared_ptr<TcpSession>);

			inline void removeTcpSession(sock_t fd);

			inline const std::shared_ptr<std::thread>& getLoopThread() const {
                return loop_thread_;
            }

			inline IoObjectImplType& getImplRef() {
				return *io_impl_.get();
			}

			inline std::shared_ptr<IoObjectImplType> getImpl() {
				return io_impl_;
			}

			std::shared_ptr<TaskExecutor> getExecutor() {
				return task_executor_; 
			}

			thread_id_t thread_id() const { return thread_id_; }
        private:
            thread_id_t thread_id_;
            std::shared_ptr<IoObjectImplType> io_impl_;
			std::shared_ptr<TaskExecutor> task_executor_;
            std::shared_ptr<std::thread> loop_thread_;
            unsigned int wait_timeout_ms_;
            std::unordered_map<sock_t, std::shared_ptr<TcpSession>> tcp_sessions_;
        };
    }
}

#include <spdnet/net/event_loop.ipp>

#endif  // SPDNET_NET_EVENT_LOOP_H_