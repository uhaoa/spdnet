#include <spdnet/net/channel.h>

namespace spdnet {
    namespace net {
        WakeupChannel::WakeupChannel(int fd) noexcept
                : fd_(fd) {

        }
    }
}