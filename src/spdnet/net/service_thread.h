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
#include <spdnet/net/channel_collector.h>

#ifdef SPDNET_PLATFORM_LINUX

#include <spdnet/net/detail/impl_linux/epoll_impl.h>

#else
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#endif

namespace spdnet {
    namespace net {
        class tcp_session;

        using tcp_enter_callback = std::function<void(std::shared_ptr<tcp_session>)>;

        class service_thread : public spdnet::net::wakeup_base, spdnet::base::noncopyable {
        public:
            explicit service_thread(unsigned int);

            ~service_thread() = default;

            void run(std::shared_ptr<bool>);

            void on_tcp_session_enter(sock_t fd, std::shared_ptr<tcp_session> tcp_session,
                                   const tcp_enter_callback &enter_callback);

            std::shared_ptr<tcp_session> get_tcp_session(sock_t fd);

            void remove_tcp_session(sock_t fd);

            void add_tcp_session(sock_t fd, std::shared_ptr<tcp_session>);

            const std::shared_ptr<std::thread> &get_thread() const {
                return thread_;
            }

            std::shared_ptr<detail::io_object_impl_type> get_impl() {
                return io_impl_;
            }

			detail::io_object_impl_type& get_impl_ref() {
				return *io_impl_;
			}


            std::shared_ptr<task_executor> get_executor() {
                return task_executor_;
            }

            thread_id_t thread_id() const { return thread_id_; }

            std::shared_ptr<channel_collector> get_channel_collector() {
                return channel_collector_;
            }

        private:
            void wakeup() override {
                if (!wakeup_flag_.exchange(true))
                    return;
                io_impl_->wakeup();
            }

            void clear_wakeup_flag() {
                wakeup_flag_ = false;
            }

        private:
            thread_id_t thread_id_;
            std::shared_ptr<detail::io_object_impl_type> io_impl_;
            std::shared_ptr<task_executor> task_executor_;
            std::atomic_bool wakeup_flag_{false};
            std::shared_ptr<channel_collector> channel_collector_;
            std::shared_ptr<std::thread> thread_;
            unsigned int wait_timeout_ms_;
            std::unordered_map<sock_t, std::shared_ptr<tcp_session>> tcp_sessions_;
        };
    }
}

#include <spdnet/net/service_thread.ipp>

#endif  // SPDNET_NET_SERVICE_THREAD_H_