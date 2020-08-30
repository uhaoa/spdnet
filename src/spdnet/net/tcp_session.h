#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/socket_ops.h>
#include <spdnet/net/detail/ssl/ssl_context.h>
#include <spdnet/net/ssl_env.h>

namespace spdnet {
    namespace net {
		namespace detail {
			class iocp_impl;
			class epoll_impl; 
			class ssl_recv_channel;
			class ssl_send_channel;
#ifdef SPDNET_PLATFORM_WINDOWS
			using io_impl_type = iocp_impl; 
			class iocp_recv_channel; 
			class iocp_send_channel; 
#else	
			using io_impl_type = epoll_impl;
			class epoll_socket_channel;
#endif
		}
		class task_executor; 
        class tcp_session : public spdnet::base::noncopyable, public std::enable_shared_from_this<tcp_session> {
        public:
            friend class service_thread;
            friend class event_service;
			friend class detail::ssl_recv_channel;
			friend class detail::ssl_send_channel;
#ifdef SPDNET_PLATFORM_WINDOWS
			friend class detail::iocp_impl;
			friend class detail::iocp_recv_channel;
			friend class detail::iocp_send_channel; 
#else
			friend class detail::epoll_impl;
			friend class detail::epoll_socket_channel;
#endif
            using tcp_data_callback = std::function<size_t(const char *, size_t len)>;
            using tcp_disconnect_callback = std::function<void(std::shared_ptr<tcp_session>)>;
			using tcp_send_complete_callback = std::function<void()>;
        public:
            tcp_session(sock_t fd, bool is_server_side, std::shared_ptr<detail::io_impl_type> io_impl, std::shared_ptr<task_executor> executor);

			~tcp_session(); 

            void post_shutdown();

            void set_disconnect_callback(tcp_disconnect_callback &&callback);

            void set_data_callback(tcp_data_callback &&callback) {
				data_callback_ = std::move(callback);
            }

            void set_max_recv_buffer_size(size_t len) {
				max_recv_buffer_size_ = len; 
            }

            void set_no_delay() {
				socket_ops::socket_no_delay(fd_);
            }

            void
            send(const char *data, size_t len, tcp_send_complete_callback &&callback = nullptr);


            sock_t sock_fd() const {
				return fd_; 
            }

			bool is_server_side() { return is_server_side_; }
        public:
            static std::shared_ptr<tcp_session>
            create(sock_t fd, bool is_server_side, std::shared_ptr<detail::io_impl_type> io_impl , std::shared_ptr<task_executor> executor);
		private:
			void close();

			bool init_ssl(std::shared_ptr<ssl_environment> ssl_env);

			bool exec_data_callback(spdnet::base::buffer& data_buf);
			struct send_packet {
				send_packet(spdnet::base::buffer* buf, tcp_send_complete_callback&& callback)
					: buffer_(buf), callback_(std::move(callback)) {}

				send_packet(const send_packet&) = default;

				send_packet(send_packet&&) = default;

				send_packet& operator=(const send_packet&) = default;

				send_packet& operator=(send_packet&&) = default;

				spdnet::base::buffer* buffer_;
				tcp_send_complete_callback callback_;
			};

        private:
			std::shared_ptr<detail::io_impl_type> io_impl_;
			std::shared_ptr<task_executor> executor_;

			sock_t fd_;
			bool is_server_side_{ false };
			tcp_disconnect_callback disconnect_callback_;
			tcp_data_callback data_callback_;
			spdnet::base::buffer recv_buffer_;
			size_t max_recv_buffer_size_ = 64 * 1024;
			std::deque<send_packet> send_packet_list_;
			std::deque<send_packet> pending_packet_list_;
			spdnet::base::spin_lock send_guard_;
			volatile bool has_closed_{ false };
			volatile bool is_post_flush_{ false };
			volatile bool is_can_write_{ true };

#if defined(SPDNET_PLATFORM_WINDOWS)
			std::shared_ptr<detail::iocp_recv_channel> recv_channel_;
			std::shared_ptr<detail::iocp_send_channel> send_channel_;
#if defined(SPDNET_USE_OPENSSL)
			std::shared_ptr<detail::iocp_ssl_recv_channel> ssl_recv_channel_;
			std::shared_ptr<detail::iocp_ssl_send_channel> ssl_send_channel_;
			std::shared_ptr<detail::ssl_context> ssl_context_; 
#endif
#else
			std::shared_ptr<detail::epoll_socket_channel> channel_;
#endif
        };

    }
}

#include <spdnet/net/tcp_session.ipp>

#endif // SPDNET_NET_TCP_SESSION_H_