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
        class ChannelCollector : public spdnet::base::NonCopyable {
        public:
            void putChannel(std::shared_ptr<detail::Channel> channel) {
                del_channel_list_.push_back(channel);
            }

            void releaseChannel() {
                del_channel_list_.clear();
            }

        private:
            std::vector<std::shared_ptr<detail::Channel>> del_channel_list_;
        };
    }

}
#endif  // SPDNET_NET_CHANNEL_COLLECTOR_H_