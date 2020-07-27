#ifndef SPDNET_NET_IOCP_IMPL_H_
#define SPDNET_NET_IOCP_IMPL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <iostream>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/base/buffer.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/detail/impl_win/iocp_wakeup_channel.h>
#include <spdnet/net/detail/impl_win/iocp_accept_channel.h>

namespace spdnet {
    namespace net {
        //service_thread
        class task_executor;
        namespace detail {
            class channel;

            class iocp_send_channel;

            class iocp_impl : public spdnet::base::noncopyable, public std::enable_shared_from_this<iocp_impl> {
            public:
                friend class iocp_recv_channel;

                friend class iocp_send_channel;

                inline explicit iocp_impl(std::shared_ptr<task_executor> task_executor,
                                          std::shared_ptr<channel_collector> channel_collector,
                                          std::function<void(sock_t)> &&socket_close_notify_cb) noexcept;

                inline virtual ~iocp_impl() noexcept;

                inline bool on_socket_enter(socket_data::ptr data);

                inline void post_flush(socket_data *data);

                inline void run_once(uint32_t timeout);

                inline bool start_accept(sock_t listen_fd, iocp_accept_channel *channel);

                inline bool async_connect(sock_t fd, const end_point &addr, channel *op);

                inline void shutdown_socket(socket_data::ptr data);

                inline void wakeup();

                spdnet::base::buffer *alloc_buffer(size_t size) {
                    return buffer_pool_.alloc_buffer(size);
                }

                void recycle_buffer(spdnet::base::buffer *buffer) {
                    buffer_pool_.recycle_buffer(buffer);
                }

            private:
                inline void close_socket(socket_data::ptr data);

            private:
                HANDLE handle_;
                iocp_wakeup_channel wakeup_op_;
                spdnet::base::buffer_pool buffer_pool_;
                std::atomic<void *> connect_ex_{nullptr};
                std::shared_ptr<task_executor> task_executor_;
                std::shared_ptr<channel_collector> channel_collector_;
                std::function<void(sock_t)> socket_close_notify_cb_;
            };

            using io_object_impl_type = iocp_impl;
        }
    }
}


#include <spdnet/net/detail/impl_win/iocp_impl.ipp>

#endif  // SPDNET_NET_IOCP_IMPL_H_