#ifndef SPDNET_NET_TCP_SESSION_IPP_
#define SPDNET_NET_TCP_SESSION_IPP_

#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/task_executor.h>
#ifdef SPDNET_PLATFORM_WINDOWS
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_send_channel.h>
#include <spdnet/net/detail/impl_win/iocp_recv_channel.h>
#ifdef SPDNET_USE_OPENSSL
#include <spdnet/net/detail/ssl/iocp_ssl_channel.h>
#endif
#else
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#endif

#include <spdnet/net/ssl_env.h>

namespace spdnet {
    namespace net {
        tcp_session::tcp_session(sock_t fd, bool is_server_side, std::shared_ptr<detail::io_impl_type> io_impl, std::shared_ptr<task_executor> executor)
			: fd_(fd), is_server_side_(is_server_side) , io_impl_(io_impl), executor_(executor) 
        {

        }

        tcp_session::~tcp_session()
        {
            if (fd_ != invalid_socket)
                socket_ops::close_socket(fd_);
        }

        std::shared_ptr<tcp_session>
		tcp_session::create(sock_t fd, bool is_server_side, std::shared_ptr<detail::io_impl_type> io_impl, std::shared_ptr<task_executor> executor){
            return std::make_shared<tcp_session>(fd, is_server_side, io_impl , executor);
        }

        void tcp_session::set_disconnect_callback(tcp_disconnect_callback &&callback) {
			/*
            auto cb = std::move(callback);
            auto this_ptr = shared_from_this();
			disconnect_callback_ = [cb, this_ptr]() mutable {
                if (cb) {
                    cb(this_ptr);
                }
                this_ptr = nullptr;
            };
			*/

			disconnect_callback_ = std::move(callback);
		}

        bool tcp_session::init_ssl(std::shared_ptr<ssl_environment> ssl_env)
        {
#if defined(SPDNET_USE_OPENSSL)
			ssl_context_ = std::make_shared<detail::ssl_context>(fd_);
			if (is_server_side_) {
				if (!ssl_context_->init_server_side(ssl_env->get_ssl_ctx())) {
                    return false; 
				}
			}
			else {
				if (!ssl_context_->init_client_side()) {
                    return false; 
				}
			}
#endif

            return true; 
        }

        bool tcp_session::exec_data_callback(spdnet::base::buffer& data_buf)
        {
			if (nullptr != data_callback_)
			{
				size_t len = data_callback_(data_buf.get_data_ptr(), data_buf.get_length());
				assert(len <= data_buf.get_length());
				if (len <= data_buf.get_length()) {
                    data_buf.remove_length(len);
				}
				else {
                    return false; 
				}
			}

            return true; 
        }

        void tcp_session::send(const char *data, size_t len, tcp_send_complete_callback &&callback) {
            if (len <= 0)
                return;
            auto buffer = io_impl_->alloc_buffer(len);
            assert(buffer);
            buffer->write(data, len);
            {
                std::lock_guard<spdnet::base::spin_lock> lck(send_guard_);
                send_packet_list_.emplace_back(send_packet(buffer, std::move(callback)));
            }
            if (is_post_flush_) {
                return;
            }
            is_post_flush_ = true;
            /*
             *   这里采用裸指针进行传递 ，主要是避免shared_ptr频繁的原子操作引起的性能上的损失 。
             *   裸指针生命周期的安全性由socket_data和channel的相互引用保证 。
             *   当socket关闭时， channel不会立即被释放 ，而channel里引用了socket_data, 所以socket_data也不会被释放。
             *   channel的直正释放是被延迟到了epoll_wait和excutor之后 ， 所以channel和socket_data释放时，
             *   send函数投递的lamba肯定已经执行完了 , 因此lamba捕获的裸指针就不存在悬指针安全问题了。
             *   这种写法看起来很不舒服 ，但为了性能只能忍一忍了 ，哈哈
            **/
			/*auto this_ptr = shared_from_this(); */
			io_impl_->post_flush(this/*this_ptr*/);
        }

        void tcp_session::post_shutdown() {
            auto this_ptr = shared_from_this();
			executor_->post([this_ptr]() {
                this_ptr->io_impl_->shutdown_session(this_ptr);
            });
        }
		void tcp_session::close() {
#if defined(SPDNET_PLATFORM_WINDOWS)
			recv_channel_ = nullptr;
			send_channel_ = nullptr;
#else
			channel_ = nullptr;
#endif
			auto this_ptr = shared_from_this();
			if (disconnect_callback_)
				disconnect_callback_(this_ptr);

			disconnect_callback_ = nullptr;
			data_callback_ = nullptr;

			has_closed_ = true;
			is_can_write_ = false;

			socket_ops::close_socket(fd_);
            fd_ = invalid_socket; 
		}
    }
}
#endif // SPDNET_NET_TCP_SESSION_IPP_