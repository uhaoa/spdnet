#ifndef SPDNET_NET_SOCKET_DATA_H_
#define SPDNET_NET_SOCKET_DATA_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/platform.h>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/net/socket_ops.h>


namespace spdnet {
    namespace net {
        namespace detail {
#if defined(SPDNET_PLATFORM_WINDOWS)
            class iocp_recv_channel;
            class iocp_send_channel;
#else

            class epoll_socket_channel;

#endif
        }
        struct socket_data : public spdnet::base::noncopyable {
        public:
            using ptr = std::shared_ptr<socket_data>;
            using tcp_data_callback = std::function<size_t(const char *, size_t len)>;
            using tcp_disconnect_callback = std::function<void()>;
            using tcp_send_complete_callback = std::function<void()>;
        public:
            socket_data(sock_t fd, bool is_server_side)
                    : fd_(fd), is_server_side_(is_server_side) {

            }

            virtual ~socket_data() {
                for (auto &packet : send_packet_list_) {
                    delete packet.buffer_;
                }
                for (auto packet : pending_packet_list_) {
                    delete packet.buffer_;
                }
                send_packet_list_.clear();
                pending_packet_list_.clear();
            }

            void set_disconnect_callback(tcp_disconnect_callback &&callback) {
                disconnect_callback_ = std::move(callback);
            }

            void set_data_callback(tcp_data_callback &&callback) {
                data_callback_ = std::move(callback);
            }

            void set_no_delay() {
                socket_ops::socket_no_delay(fd_);
            }

            sock_t sock_fd() const {
                return fd_;
            }

            void set_max_recv_buffer_size(size_t max_size) {
                max_recv_buffer_size_ = max_size;
            }

            bool is_server_side() { return is_server_side_; }

            void close() {
#if defined(SPDNET_PLATFORM_WINDOWS)
                recv_channel_ = nullptr;
                send_channel_ = nullptr;
#else
                channel_ = nullptr;
#endif
                if (disconnect_callback_)
                    disconnect_callback_();

                disconnect_callback_ = nullptr;
                data_callback_ = nullptr;

                has_closed_ = true;
                is_can_write_ = false;

                socket_ops::close_socket(fd_);
            }

        public:
            struct send_packet {
                send_packet(spdnet::base::buffer *buf, tcp_send_complete_callback &&callback)
                        : buffer_(buf), callback_(std::move(callback)) {}

                send_packet(const send_packet &) = default;

                send_packet(send_packet &&) = default;

                send_packet &operator=(const send_packet &) = default;

                send_packet &operator=(send_packet &&) = default;

                spdnet::base::buffer *buffer_;
                tcp_send_complete_callback callback_;
            };

        public:
            sock_t fd_;
            bool is_server_side_{false};
            tcp_disconnect_callback disconnect_callback_;
            tcp_data_callback data_callback_;
            spdnet::base::buffer recv_buffer_;
            size_t max_recv_buffer_size_ = 64 * 1024;
            std::deque<send_packet> send_packet_list_;
            std::deque<send_packet> pending_packet_list_;
            spdnet::base::spin_lock send_guard_;
            volatile bool has_closed_{false};
            volatile bool is_post_flush_{false};
            volatile bool is_can_write_{true};

#if defined(SPDNET_PLATFORM_WINDOWS)
            std::shared_ptr<detail::iocp_recv_channel> recv_channel_;
            std::shared_ptr<detail::iocp_send_channel> send_channel_;
#else
            std::shared_ptr<detail::epoll_socket_channel> channel_;
#endif
        };
    }

}
#endif  // SPDNET_NET_SOCKET_DATA_H_