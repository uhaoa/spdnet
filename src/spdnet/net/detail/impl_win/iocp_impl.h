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
#include <spdnet/net/end_point.h>
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/detail/impl_win/iocp_wakeup_channel.h>
#include <spdnet/net/detail/impl_win/iocp_accept_channel.h>
namespace spdnet {
    namespace net {
        class task_executor;
		class tcp_session; 
		class channel_collector; 
		namespace detail {
            class channel;
            class iocp_send_channel;

            class iocp_impl : public spdnet::base::noncopyable, public std::enable_shared_from_this<iocp_impl> {
            public:
                friend class iocp_recv_channel;

                friend class iocp_send_channel;

                explicit iocp_impl(std::shared_ptr<task_executor> task_executor,
                                          std::shared_ptr<channel_collector> channel_collector);

                virtual ~iocp_impl() noexcept;

                bool on_socket_enter(std::shared_ptr<tcp_session> session);

                void post_flush(tcp_session *session);

                void run_once(uint32_t timeout);

                bool start_accept(sock_t listen_fd, iocp_accept_channel *channel);

                bool async_connect(sock_t fd, const end_point &addr, channel *op);

                void shutdown_session(std::shared_ptr<tcp_session> session);

                void wakeup();

                spdnet::base::buffer *alloc_buffer(size_t size) {
                    return buffer_pool_.alloc_buffer(size);
                }

                void recycle_buffer(spdnet::base::buffer *buffer) {
                    buffer_pool_.recycle_buffer(buffer);
                }

            private:
                void close_socket(std::shared_ptr<tcp_session> session);

            private:
                HANDLE handle_;
                iocp_wakeup_channel wakeup_op_;
                spdnet::base::buffer_pool buffer_pool_;
                std::atomic<void *> connect_ex_{nullptr};
                std::shared_ptr<task_executor> task_executor_;
                std::shared_ptr<channel_collector> channel_collector_;
            };

            using io_impl_type = iocp_impl;
        }
    }
}


#include <spdnet/net/detail/impl_win/iocp_impl.ipp>

#endif  // SPDNET_NET_IOCP_IMPL_H_