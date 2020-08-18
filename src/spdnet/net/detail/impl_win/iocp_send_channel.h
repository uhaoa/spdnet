#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SEND_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SEND_CHANNEL_H_

#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>
#include <spdnet/base/buffer_pool.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class iocp_send_channel : public socket_channel {
            public:
                iocp_send_channel(std::shared_ptr<tcp_session> session, std::shared_ptr<io_impl_type> io_impl)
                        : socket_channel(session, io_impl) {

                }

                void flush_buffer() {
                    {
                        std::lock_guard<spdnet::base::spin_lock> lck(session_->send_guard_);
                        if (SPDNET_PREDICT_TRUE(session_->pending_packet_list_.empty())) {
							session_->pending_packet_list_.swap(session_->send_packet_list_);
                        } else {
                            for (const auto &packet : session_->send_packet_list_)
								session_->pending_packet_list_.push_back(packet);
							session_->send_packet_list_.clear();
                        }
                    }

                    constexpr size_t MAX_BUF_CNT = 1024;
                    WSABUF send_buf[MAX_BUF_CNT];

                    size_t cnt = 0;
                    size_t prepare_send_len = 0;
                    for (const auto &packet : session_->pending_packet_list_) {
                        send_buf[cnt].buf = packet.buffer_->get_data_ptr();
                        send_buf[cnt].len = packet.buffer_->get_length();
                        cnt++;
                        if (cnt >= MAX_BUF_CNT)
                            break;
                    }
                    assert(cnt > 0);

                    DWORD send_len = 0;
                    const int result = ::WSASend(session_->sock_fd(),
                                                 send_buf,
                                                 cnt,
                                                 &send_len,
                                                 0,
                                                 (LPOVERLAPPED)session_->send_channel_.get(),
                                                 0);
                    DWORD last_error = current_errno();
                    if (result != 0 && last_error != WSA_IO_PENDING) {
                        io_impl_->close_socket(session_);
                    }
                }

            private:
                void do_complete(size_t bytes_transferred, std::error_code ec) override {
                    if (bytes_transferred == 0 || ec) {
                        io_impl_->close_socket(session_);
                    } else {
                        auto send_len = bytes_transferred;
                        for (auto iter = session_->pending_packet_list_.begin();
                             iter != session_->pending_packet_list_.end();) {
                            auto &packet = *iter;
                            if (SPDNET_PREDICT_TRUE(packet.buffer_->get_length() <= send_len)) {
                                send_len -= packet.buffer_->get_length();
                                packet.buffer_->clear();
                                io_impl_->recycle_buffer(packet.buffer_);
                                if (packet.callback_)
                                    packet.callback_();
                                iter = session_->pending_packet_list_.erase(iter);
                            } else {
                                packet.buffer_->remove_length(send_len);
                                break;
                            }

                        }
                        {
                            std::unique_lock<spdnet::base::spin_lock> lck(session_->send_guard_);
                            if (session_->send_packet_list_.empty() && session_->pending_packet_list_.empty()) {
								session_->is_post_flush_ = false;
                            } else {
                                lck.unlock();
                                flush_buffer();
                            }
                        }

                    }
                }
            };
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SEND_CHANNEL_H_