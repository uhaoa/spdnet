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
                iocp_send_channel(socket_data::ptr data, std::shared_ptr<io_object_impl_type> io_impl)
                        : socket_channel(data, io_impl) {

                }

                void flush_buffer() {
                    {
                        std::lock_guard<spdnet::base::spin_lock> lck(data_->send_guard_);
                        if (SPDNET_PREDICT_TRUE(data_->pending_buffer_list_.empty())) {
                            data_->pending_buffer_list_.swap(data_->send_buffer_list_);
                        } else {
                            for (const auto buffer : data_->send_buffer_list_)
                                data_->pending_buffer_list_.push_back(buffer);
                            data_->send_buffer_list_.clear();
                        }
                    }

                    constexpr size_t MAX_BUF_CNT = 1024;
                    WSABUF send_buf[MAX_BUF_CNT];

                    size_t cnt = 0;
                    size_t prepare_send_len = 0;
                    for (const auto buffer : data_->pending_buffer_list_) {
                        send_buf[cnt].buf = buffer->get_data_ptr();
                        send_buf[cnt].len = buffer->get_length();
                        cnt++;
                        if (cnt >= MAX_BUF_CNT)
                            break;
                    }
                    assert(cnt > 0);

                    DWORD send_len = 0;
                    const int result = ::WSASend(data_->sock_fd(),
                                                 send_buf,
                                                 cnt,
                                                 &send_len,
                                                 0,
                                                 (LPOVERLAPPED) data_->send_channel_.get(),
                                                 0);
                    DWORD last_error = current_errno();
                    if (result != 0 && last_error != WSA_IO_PENDING) {
                        io_impl_->close_socket(data_);
                    }
                }

            private:
                void do_complete(size_t bytes_transferred, std::error_code ec) override {
                    if (bytes_transferred == 0 || ec) {
                        io_impl_->close_socket(data_);
                    } else {
                        auto send_len = bytes_transferred;
                        for (auto iter = data_->pending_buffer_list_.begin();
                             iter != data_->pending_buffer_list_.end();) {
                            auto buffer = *iter;
                            if (SPDNET_PREDICT_TRUE(buffer->get_length() <= send_len)) {
                                send_len -= buffer->get_length();
                                buffer->clear();
                                io_impl_->recycle_buffer(buffer);
                                iter = data_->pending_buffer_list_.erase(iter);
                            } else {
                                buffer->remove_length(send_len);
                                break;
                            }

                        }
                        {
                            std::unique_lock<spdnet::base::spin_lock> lck(data_->send_guard_);
                            if (data_->send_buffer_list_.empty() && data_->pending_buffer_list_.empty()) {
                                data_->is_post_flush_ = false;
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