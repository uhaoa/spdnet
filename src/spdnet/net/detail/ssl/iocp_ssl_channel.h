#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SSL_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SSL_CHANNEL_H_

#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/detail/ssl/ssl_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class iocp_ssl_send_channel : public ssl_send_channel {
            public:
                iocp_ssl_send_channel(std::shared_ptr<tcp_session> session, std::shared_ptr<io_impl_type> io_impl)
                        : ssl_send_channel(session, io_impl) {

                }

            private:
                void post_write_event() override
                {
					static CHAR send_tmp_data_[] = { 0 };
					static WSABUF send_buf_ = { 0, send_tmp_data_ };


					DWORD send_len = 0;
					const int result = ::WSASend(session_->sock_fd() , 
                        &send_buf_ , 
						1, 
						&send_len,
						0,
						(LPOVERLAPPED)this,
						0);
					DWORD last_error = current_errno();
					if (result != 0 && last_error != WSA_IO_PENDING) {
						io_impl_->close_socket(session_);
					}
                }
                void do_complete(size_t bytes_transferred, std::error_code ec) override {
                    if (/*bytes_transferred == 0 || */ec) {
                        io_impl_->close_socket(session_);
                    } else {
                        flush_buffer();
                    }
                }
            };

            class iocp_ssl_recv_channel : public ssl_recv_channel {
            public:
                iocp_ssl_recv_channel(std::shared_ptr<tcp_session> session, std::shared_ptr<io_impl_type> io_impl)
                    : ssl_recv_channel(session, io_impl) 
                {

                }

                void start_recv() override {
					static CHAR recv_tmp_data_[] = { 0 };
					static WSABUF recv_buf_ = { 0, recv_tmp_data_ };

					DWORD bytes_transferred = 0;
					DWORD recv_flags = 0;
					reset();
					int result = ::WSARecv(session_->sock_fd(), &recv_buf_, 1, &bytes_transferred, &recv_flags, (LPOVERLAPPED)
						this, 0);
					DWORD last_error = ::WSAGetLastError();
					if (result != 0 && last_error != WSA_IO_PENDING) {
						io_impl_->close_socket(session_);
					}
                }

            private:
                void do_complete(size_t bytes_transferred, std::error_code ec) override {
                    if (/*bytes_transferred == 0 || */ec) {
                        io_impl_->close_socket(session_);
                    }
                    else {
                        do_recv();
                    }
                }
            };
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SSL_CHANNEL_H_