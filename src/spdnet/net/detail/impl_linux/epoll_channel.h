#ifndef SPDNET_NET_EPOLL_CHANNEL_H_
#define SPDNET_NET_EPOLL_CHANNEL_H_

namespace spdnet {
    namespace net {
        namespace detail {
            class channel {
            public:
                virtual ~channel() noexcept {}

                virtual void on_send() = 0;

                virtual void on_recv() = 0;

                virtual void on_close() = 0;

            };

        }
    }

}

#endif  // SPDNET_NET_EPOLL_CHANNEL_H_