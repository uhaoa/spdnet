#ifndef SPDNET_NET_EPOLL_SSL_CHANNEL_H_
#define SPDNET_NET_EPOLL_SSL_CHANNEL_H_

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#include <spdnet/net/detail/impl_linux/epoll_channel.h>
#include <spdnet/net/detail/ssl/ssl_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class epoll_ssl_channel : public ssl_recv_channel , public ssl_send_channel {
            public:
                friend class epoll_impl;

                epoll_ssl_channel(std::shared_ptr<epoll_impl> impl, std::shared_ptr<tcp_session> session)
                   // :ssl_recv_channel(session , impl) , ssl_send_channel(session , impl)
                {
                    io_impl_ = impl;
                    session_ = session;
                }

            private:
                void on_send() override {
                    cancel_write_event();
					session_->is_can_write_ = true;
                    flush_buffer();
                }

                void on_recv() override {
                    do_recv();
                }

                void on_close() override {
                    io_impl_->close_socket(session_);
                }

                void post_write_event() override
				{
					struct epoll_event event { 0, { nullptr } };
					event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
					event.data.ptr = this;
                    ::epoll_ctl(io_impl_->epoll_fd(), EPOLL_CTL_MOD, session_->sock_fd(), &event);

                }

                void cancel_write_event()
                {
					struct epoll_event event { 0, { nullptr } };
					event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
					event.data.ptr = this;
					::epoll_ctl(io_impl_->epoll_fd(), EPOLL_CTL_MOD, session_->sock_fd(), &event);
                }
            private:
              //  std::shared_ptr<ssl_recv_channel> recv_channel_;
              //  std::shared_ptr<ssl_send_channel> send_channel_;
            };

        }
    }

}

#endif  // SPDNET_NET_EPOLL_SSL_CHANNEL_H_
