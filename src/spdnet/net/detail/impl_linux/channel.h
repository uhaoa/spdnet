#ifndef SPDNET_NET_CHANNEL_H_
#define SPDNET_NET_CHANNEL_H_

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#include <spdnet/net/event_loop.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class Channel {
            public:
                virtual ~Channel() noexcept {}

                virtual void trySend() = 0;

                virtual void tryRecv() = 0;

                virtual void onClose() = 0;
            };

            class TcpSessionChannel : public Channel {
            public:
                friend class EpollImpl;

                TcpSessionChannel(std::shared_ptr<EventLoop> loop, SocketImplData& data)
                    :loop_(loop), data_(data)
				{

                }

                void trySend() override {
                    loop_->getImpl().flushBuffer(data_);
                    loop_->getImpl().cancelWriteEvent(data_);
                }

                void tryRecv() override {
                    loop_->getImpl().doRecv(data_);
                }

                void onClose() override {
                    loop_->getImpl().closeSocket(data_);
                }

            private:
                std::shared_ptr<EventLoop> loop_;
				SocketImplData& data_;
            };

            class WakeupChannel : public Channel {
            public:
                explicit WakeupChannel(int fd) noexcept
                        : fd_(fd) {}

                bool wakeup() {
                    int data = 1;
                    return ::write(fd_, &data, sizeof(data)) == 0;
                }

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
                int fd_;
            };

        }
    }

}

#endif  // SPDNET_NET_CHANNEL_H_