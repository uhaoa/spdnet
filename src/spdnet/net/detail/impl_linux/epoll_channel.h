#ifndef SPDNET_NET_EPOLL_CHANNEL_H_
#define SPDNET_NET_EPOLL_CHANNEL_H_

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

        }
    }

}

#endif  // SPDNET_NET_EPOLL_CHANNEL_H_