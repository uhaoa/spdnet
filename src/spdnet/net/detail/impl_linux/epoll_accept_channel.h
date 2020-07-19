#ifndef SPDNET_NET_EPOLL_ACCEPT_CHANNEL_H_
#define SPDNET_NET_EPOLL_ACCEPT_CHANNEL_H_

#include <iostream>
#include <spdnet/net/socket_ops.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_linux/epoll_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class epoll_accept_channel : public channel {
            public:
                epoll_accept_channel(sock_t listen_fd, std::function<void(sock_t fd)> &&success_cb)
                        : listen_fd_(listen_fd), success_cb_(success_cb),
                          idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {

                }

                void on_send() override {}

                void on_close() override {}

                void on_recv() override {
                    sock_t accept_fd = ::accept(listen_fd_, nullptr, nullptr);
                    if (accept_fd == -1) {
                        if (current_errno() == EMFILE) {
                            ::close(idle_fd_);
                            accept_fd = ::accept(listen_fd_, nullptr, nullptr);
                            ::close(accept_fd);
                            idle_fd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
                        }
                        std::cerr << "accept error . errno:" << current_errno() << std::endl;
                        return;
                    }
                    success_cb_(accept_fd);
                }

                ~epoll_accept_channel() {
                    socket_ops::close_socket(idle_fd_);
                }

            private:
                sock_t listen_fd_{invalid_socket};
                std::function<void(sock_t fd)> success_cb_;
                sock_t idle_fd_;
            };


            using accept_channel_impl = epoll_accept_channel;

        }
    }
}

#endif // SPDNET_NET_EPOLL_ACCEPT_CHANNEL_H_