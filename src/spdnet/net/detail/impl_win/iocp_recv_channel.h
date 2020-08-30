#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP_RECV_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP_RECV_CHANNEL_H_

#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <iostream>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class iocp_recv_channel : public socket_channel {
            public:
                iocp_recv_channel(std::shared_ptr<tcp_session> session, std::shared_ptr<iocp_impl> impl)
                        : socket_channel(session, impl) {

                }

                void start_recv() {
                    buf_.len = session_->recv_buffer_.get_write_valid_count();
                    buf_.buf = session_->recv_buffer_.get_write_ptr();

                    DWORD bytes_transferred = 0;
                    DWORD recv_flags = 0;
                    reset();
                    int result = ::WSARecv(session_->sock_fd(), &buf_, 1, &bytes_transferred, &recv_flags, (LPOVERLAPPED)
                    this, 0);
                    DWORD last_error = ::WSAGetLastError();
                    if (result != 0 && last_error != WSA_IO_PENDING) {
                        io_impl_->close_socket(session_);
                    }
                }

            private:
                void do_complete(size_t bytes_transferred, std::error_code ec) override {
                    bool force_close = false;
                    if (bytes_transferred == 0 || ec) {
                        // eof 
                        force_close = true;
                    } else {
                        auto &recv_buffer = session_->recv_buffer_;
                        auto post_len = recv_buffer.get_write_valid_count();
                        recv_buffer.add_write_pos(bytes_transferred);
						if (!session_->exec_data_callback(recv_buffer)) {
							force_close = true;
						}
                        if (post_len == bytes_transferred) {
                            recv_buffer.adjust_capacity(session_->max_recv_buffer_size_); 
                        }

                        recv_buffer.try_adjust_to_head();
                    }


                    if (force_close)
                        io_impl_->close_socket(session_);
                    else
                        this->start_recv();
                }

            private:
                WSABUF buf_ = {0, 0};
            };
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP_RECV_CHANNEL_H_