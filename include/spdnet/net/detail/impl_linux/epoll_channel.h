#ifndef SPDNET_NET_EPOLL_CHANNEL_H_
#define SPDNET_NET_EPOLL_CHANNEL_H_

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class Channel {
            public:
                virtual ~Channel() noexcept {}

                virtual void trySend() = 0;

                virtual void tryRecv() = 0;

                virtual void onClose() = 0;

                // whether the session is valid
                volatile bool cancel_token_ = false; 
            };

        }
    }

}

#endif  // SPDNET_NET_EPOLL_CHANNEL_H_