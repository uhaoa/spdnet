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
#include <spdnet/net/tcp_session.h>
#include <spdnet/base/buffer_pool.h>

namespace spdnet {
    namespace net {
        using namespace base;
        namespace detail {
            class EpollImpl;
        }

        class SPDNET_EXPORT EventLoop : public base::NonCopyable {
        public:
            using AsynLoopTask = std::function<void()>;
        public:
            explicit EventLoop(unsigned int) noexcept;

            ~EventLoop() = default;

            void run(std::shared_ptr<bool>);

            inline bool isInLoopThread() const {
                return thread_id_ == current_thread::tid();
            }

            void post(AsynLoopTask&& task);

            void onTcpSessionEnter(TcpSession::Ptr tcp_session, const TcpSession::TcpEnterCallback& enter_callback);

            TcpSession::Ptr getTcpSession(sock_t fd);

            void addTcpSession(TcpSession::Ptr);

            void removeTcpSession(sock_t fd);

            const std::shared_ptr<std::thread>& getLoopThread() const {
                return loop_thread_;
            }

            spdnet::base::Buffer* allocBufferBySize(size_t size) {
                return buffer_pool_.allocBufferBySize(size);
            }

            void recycleBuffer(spdnet::base::Buffer* buffer) {
                buffer_pool_.recycleBuffer(buffer);
            }
#if defined SPDNET_PLATFORM_LINUX
            detail::EpollImpl& getImpl() {
                return *io_impl_.get();
            }
#else
			detail::IocpImpl& getImpl() {
				return *io_impl_.get();
			}
#endif
        private:
            void execAsyncTasks();

        private:
            current_thread::THREAD_ID_TYPE thread_id_;
#if defined SPDNET_PLATFORM_LINUX
            std::shared_ptr<detail::EpollImpl> io_impl_;
#else
            std::shared_ptr<detail::IocpImpl> io_impl_;
#endif
            std::mutex task_mutex_;
            std::vector<AsynLoopTask> async_tasks;
            std::vector<AsynLoopTask> tmp_async_tasks;
            std::shared_ptr<std::thread> loop_thread_;
            unsigned int wait_timeout_ms_;
            std::unordered_map<sock_t, TcpSession::Ptr> tcp_sessions_;
            spdnet::base::BufferPool buffer_pool_;
        };
    }
}
#endif  // SPDNET_NET_EVENT_LOOP_H_