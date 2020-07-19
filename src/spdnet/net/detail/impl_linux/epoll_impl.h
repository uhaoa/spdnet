#ifndef SPDNET_NET_EPOLL_IMPL_H_
#define SPDNET_NET_EPOLL_IMPL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <iostream>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/base/buffer.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/end_point.h>
#include <spdnet/net/channel_collector.h>
#include <spdnet/net/detail/impl_linux/epoll_wakeup_channel.h>

namespace spdnet {
    namespace net {
        class task_executor;

        class async_connector;
        namespace detail {
            class channel;

            class epoll_impl : public spdnet::base::noncopyable, public std::enable_shared_from_this<epoll_impl> {
            public:
                friend class epoll_socket_channel;

                explicit epoll_impl(std::shared_ptr<task_executor> task_executor,
                                   std::shared_ptr<channel_collector> channel_collector,
                                   std::function<void(sock_t)> &&socket_close_notify_cb);

                virtual ~epoll_impl() noexcept;

                bool on_socket_enter(socket_data::ptr data);

                void run_once(uint32_t timeout);

                void send(socket_data *socket_data, const char *data, size_t len);

                void shutdown_socket(socket_data::ptr data);

                int epoll_fd() const { return epoll_fd_; }

                bool link_channel(int fd, const channel *channel, uint32_t events);

                bool start_accept(sock_t listen_fd, const channel *channel);

                bool async_connect(sock_t client_fd, const end_point &addr, channel *channel);

                void wakeup();

                spdnet::base::buffer *alloc_buffer(size_t size) {
                    return buffer_pool_.alloc_buffer(size);
                }

                void recycle_buffer(spdnet::base::buffer *buffer) {
                    buffer_pool_.recycle_buffer(buffer);
                }

            private:
                void close_socket(socket_data::ptr data);

                void add_write_event(socket_data::ptr data);

                void cancel_write_event(socket_data::ptr data);

                void do_recv(socket_data::ptr data);

            private:
                int epoll_fd_;
                epoll_wakeup_channel wakeup_;
                spdnet::base::buffer_pool buffer_pool_;
                std::vector<epoll_event> event_entries_;
                std::shared_ptr<task_executor> task_executor_;
                std::shared_ptr<channel_collector> channel_collector_;
                std::function<void(sock_t)> socket_close_notify_cb_;
            };

            using io_object_impl_type = epoll_impl;
        }
    }
}

#include <spdnet/net/detail/impl_linux/epoll_impl.ipp>

#endif  // SPDNET_NET_EPOLL_IMPL_H_