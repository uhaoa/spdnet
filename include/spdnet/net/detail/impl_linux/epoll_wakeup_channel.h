#ifndef SPDNET_NET_EPOLL_WAKEUP_CHANNEL_H_
#define SPDNET_NET_EPOLL_WAKEUP_CHANNEL_H_

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_linux/epoll_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class EpollWakeupChannel : public Channel {
            public:
                explicit EpollWakeupChannel() noexcept
                        : fd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {}

                ~EpollWakeupChannel()
                {
                    socket_ops::closeSocket(fd_); 
                }
                bool wakeup() {
                    int data = 1;
                    return ::write(fd_, &data, sizeof(data)) == 0;
                }

                sock_t eventfd() const { return fd_; }
            private:
                void trySend() override {

                }

                void tryRecv() override {
                    char buf[1024]{0};
                    while (true) {
                        auto ret = ::read(fd_, buf, sizeof(buf));
                        if (ret == -1 || static_cast<size_t>(ret) < sizeof(buf))
                            break;
                    }
                }

                void onClose() override {

                }

            private:
                sock_t fd_;
            };

        }
    }

}

#endif  // SPDNET_NET_EPOLL_WAKEUP_CHANNEL_H_