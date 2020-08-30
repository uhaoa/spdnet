#ifndef SPDNET_NET_DETAIL_IMPL_SSL_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_SSL_CHANNEL_H_

#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#if defined(SPDNET_PLATFORM_WINDOWS)
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>
#else
#include <spdnet/net/detail/impl_linux/epoll_channel.h>
#endif
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class ssl_send_channel
#if defined(SPDNET_PLATFORM_WINDOWS)
				: public socket_channel
#else
				: virtual public socket_channel
#endif

            {
            public:
                ssl_send_channel() = default; 

                ssl_send_channel(std::shared_ptr<tcp_session> session, std::shared_ptr<io_impl_type> io_impl) 
                    :socket_channel(session, io_impl)
                {

                }
#if defined(SPDNET_USE_OPENSSL)

                void flush_buffer() {
                    if (!session_->ssl_context_->has_already_handshake()) {
                        session_->ssl_context_->try_start_ssl_handshake(); 
                        if (!session_->ssl_context_->has_already_handshake()) {
                            post_write_event();
                            return;
                        }
                    }
                    {
                        std::lock_guard<spdnet::base::spin_lock> lck(session_->send_guard_);
                        if (SPDNET_PREDICT_TRUE(session_->pending_packet_list_.empty())) {
                            session_->pending_packet_list_.swap(session_->send_packet_list_);
                        } else {
                            for (const auto& packet : session_->send_packet_list_)
                                session_->pending_packet_list_.push_back(packet);
                            session_->send_packet_list_.clear();
                        }
                    }
                    for (const auto& packet : session_->pending_packet_list_) {
                        buffer_.write(packet.buffer_->get_data_ptr(), packet.buffer_->get_length()); 
                    }
                    bool force_close = false; 
                    int send_len = SSL_write(session_->ssl_context_->get_ssl(), buffer_.get_data_ptr(), buffer_.get_length());
                    if (send_len <= 0) {
						if ((SSL_get_error(session_->ssl_context_->get_ssl(), send_len) != SSL_ERROR_WANT_WRITE) &&
							(current_errno() != WSAEWOULDBLOCK))
						{
                            force_close = true; 
						}
                    }
                    else {
						for (auto iter = session_->pending_packet_list_.begin();
							iter != session_->pending_packet_list_.end();) {
							auto& packet = *iter;
							if (SPDNET_PREDICT_TRUE(packet.buffer_->get_length() <= send_len)) {
								send_len -= packet.buffer_->get_length();
								packet.buffer_->clear();
								io_impl_->recycle_buffer(packet.buffer_);
								if (packet.callback_)
									packet.callback_();
								iter = session_->pending_packet_list_.erase(iter);
							}
							else {
								packet.buffer_->remove_length(send_len);
								break;
							}

						}
                    }

                    buffer_.clear();
                    if (force_close) {
                        io_impl_->close_socket(session_);
                    }
                    else if (!session_->pending_packet_list_.empty()){
                        post_write_event(); 
                    }
                    else {
                        session_->is_post_flush_ = false; 
                        if (!session_->send_packet_list_.empty()) {
                            post_write_event();
                        }
                    }
                }

                virtual void post_write_event() = 0;
            private:
                spdnet::base::buffer buffer_; 
#endif
            };

            class ssl_recv_channel 
#if defined(SPDNET_PLATFORM_WINDOWS)
                : public socket_channel
#else
                : virtual public socket_channel
#endif
            {
            public:
                ssl_recv_channel() = default;

                ssl_recv_channel(std::shared_ptr<tcp_session> session, std::shared_ptr<io_impl_type> io_impl)
					: socket_channel(session, io_impl)
                {

                }
#if defined(SPDNET_USE_OPENSSL)
                void do_recv() {
                    assert(session_->ssl_context_ != nullptr);
                    {
                        if (!session_->ssl_context_->has_already_handshake()) {
                            session_->ssl_context_->try_start_ssl_handshake();
							if (!session_->ssl_context_->has_already_handshake()) {
                                start_recv();
                                return; 
							}
                        }
                        auto& recv_buffer = session_->recv_buffer_;
                        bool force_close = false; 
                        while (true) {
                            size_t write_valid_count = recv_buffer.get_write_valid_count(); 
                            int ret_len = SSL_read(session_->ssl_context_->get_ssl(),
                                recv_buffer.get_write_ptr(), write_valid_count);
                            if (ret_len == 0) {
                                force_close = true; 
                                break; 
                            }
                            else if (ret_len < 0) {
								if ((SSL_get_error(session_->ssl_context_->get_ssl(), ret_len) != SSL_ERROR_WANT_READ) &&
									(current_errno() != WSAEWOULDBLOCK))
								{
                                    force_close = true;
								}
                                break;
                            }
                            else {
                                recv_buffer.add_write_pos(ret_len);
                                if (!session_->exec_data_callback(recv_buffer)) {
                                    force_close = true;
                                }
                                recv_buffer.try_adjust_to_head(); 
								if (write_valid_count == ret_len) {
                                    recv_buffer.adjust_capacity(session_->max_recv_buffer_size_); 
								}
                                else {
                                    break; 
                                }
                            }
                        }

                        if (force_close) {
                            io_impl_->close_socket(session_);
                        }
                        else {
                            start_recv();
                        }
                    }
                }

                virtual void start_recv() {};
#endif
            };
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_SSL_CHANNEL_H_