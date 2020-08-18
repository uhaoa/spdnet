#ifndef SPDNET_NET_SERVICE_THREAD_IPP_
#define SPDNET_NET_SERVICE_THREAD_IPP_

#include <memory>
#include <iostream>
#include <cassert>
#include <spdnet/net/service_thread.h>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session_mgr.h>

namespace spdnet {
    namespace net {

        service_thread::service_thread(unsigned int wait_timeout_ms)
                : wait_timeout_ms_(wait_timeout_ms) {
            task_executor_ = std::make_shared<task_executor>(this);
            channel_collector_ = std::make_shared<channel_collector>();
            auto impl = std::make_shared<detail::io_impl_type>(task_executor_, channel_collector_);
            io_impl_ = impl;
        }

        void
        service_thread::on_tcp_session_enter(sock_t fd, std::shared_ptr<tcp_session> tcp_session,
                                             const tcp_enter_callback &enter_callback) {
            if (!io_impl_->on_socket_enter(tcp_session)) {
                return;
            }
            if (nullptr != enter_callback)
                enter_callback(tcp_session);
			tcp_session_mgr::instance().add(fd, std::move(tcp_session));
        }

        void service_thread::run(std::shared_ptr<bool> is_run) {
            thread_ = std::make_shared<std::thread>([is_run, this]() {
                thread_id_ = current_thread::tid();
                task_executor_->set_thread_id(thread_id_);
                while (*is_run) {
                    // run io
                    io_impl_->run_once(wait_timeout_ms_);

                    clear_wakeup_flag();

                    // do task
                    task_executor_->run();
                    // release channel
                    channel_collector_->release_channel();
                }
            });
        }
    }
}

#endif // SPDNET_NET_SERVICE_THREAD_IPP_