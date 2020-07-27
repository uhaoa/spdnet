#ifndef SPDNET_NET_TCP_SESSION_IPP_
#define SPDNET_NET_TCP_SESSION_IPP_

#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/service_thread.h>

namespace spdnet {
    namespace net {
        tcp_session::tcp_session(sock_t fd, bool is_server_side, std::shared_ptr<service_thread> service_thread)
                : service_thread_(service_thread) {
            socket_data_ = std::make_shared<socket_data>(fd, is_server_side);
        }

        std::shared_ptr<tcp_session>
        tcp_session::create(sock_t fd, bool is_server_side, std::shared_ptr<service_thread> service_thread) {
            return std::make_shared<tcp_session>(fd, is_server_side, service_thread);
        }

        void tcp_session::set_disconnect_callback(tcp_disconnect_callback &&callback) {
            auto cb = std::move(callback);
            auto this_ptr = shared_from_this();
            socket_data_->set_disconnect_callback([cb, this_ptr]() mutable {
                if (cb) {
                    cb(this_ptr);
                }
                this_ptr = nullptr;
                /*this_ptr->loop_owner_->remove_tcp_session(this_ptr->sock_fd());*/
            });
        }

        void tcp_session::send(const char *data, size_t len , socket_data::tcp_send_complete_callback&& callback) {
            if (len <= 0)
                return;
			auto& impl_ref = service_thread_->get_impl_ref(); 
			auto buffer = impl_ref.alloc_buffer(len);
			assert(buffer);
			buffer->write(data, len);
			{
				std::lock_guard<spdnet::base::spin_lock> lck(socket_data_->send_guard_);
				socket_data_->send_packet_list_.emplace_back(socket_data::send_packet(buffer  ,std::move(callback)));
			}
			if (socket_data_->is_post_flush_) {
				return;
			}
			socket_data_->is_post_flush_ = true;
            /*
             *   这里采用裸指针进行传递 ，主要是避免shared_ptr频繁的原子操作引起的性能上的损失 。
             *   裸指针生命周期的安全性由socket_data和channel的相互引用保证 。
             *   当socket关闭时， channel不会立即被释放 ，而channel里引用了socket_data, 所以socket_data也不会被释放。
             *   channel的直正释放是被延迟到了epoll_wait和excutor之后 ， 所以channel和socket_data释放时，
             *   send函数投递的lamba肯定已经执行完了 , 因此lamba捕获的裸指针就不存在悬指针安全问题了。
             *   这种写法看起来很不舒服 ，但为了性能只能忍一忍了 ，哈哈
            **/
			impl_ref.post_flush(socket_data_.get());
        }

        void tcp_session::post_shutdown() {
            auto this_ptr = shared_from_this();
            service_thread_->get_executor()->post([this_ptr]() {
                this_ptr->service_thread_->get_impl()->shutdown_socket(this_ptr->socket_data_);
            });
        }

    }
}
#endif // SPDNET_NET_TCP_SESSION_IPP_