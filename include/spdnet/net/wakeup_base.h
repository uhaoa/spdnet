#ifndef SPDNET_NET_WAKEUP_BASE_H
#define SPDNET_NET_WAKEUP_BASE_H

namespace spdnet {
    namespace net {
        class WakeupBase {
        public:
            virtual void wakeup() = 0; 
        };
    }
}
#endif // SPDNET_NET_WAKEUP_BASE_H
