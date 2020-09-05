#ifndef SPDNET_NET_CHANNEL_COLLECTOR_H_
#define SPDNET_NET_CHANNEL_COLLECTOR_H_

#include <memory>
#include <vector>
#include <spdnet/base/platform.h>
#include <spdnet/base/noncopyable.h>

#if defined(SPDNET_PLATFORM_WINDOWS)
#include <spdnet/net/detail/impl_win/iocp_channel.h>
#else

#include <spdnet/net/detail/impl_linux/epoll_channel.h>

#endif

namespace spdnet {
    namespace net {
        class channel_collector : public spdnet::base::noncopyable {
        public:
            void put_channel(std::shared_ptr<detail::channel> channel) {
                if (channel)
                    del_channel_list_.push_back(channel);
            }

            void release_channel() {
                del_channel_list_.clear();
            }

        private:
            std::vector<std::shared_ptr<detail::channel>> del_channel_list_;
        };
    }

}
#endif  // SPDNET_NET_CHANNEL_COLLECTOR_H_