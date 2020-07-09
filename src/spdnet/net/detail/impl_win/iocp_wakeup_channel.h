#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP__WAKEUP_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP__WAKEUP_CHANNEL_H_

#include <system_error>
#include <spdnet/net/detail/impl_win/iocp_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class IocpWakeupChannel : public Channel {
            public:
                IocpWakeupChannel(HANDLE handle)
                        : handle_(handle) {

                }

                void wakeup() {
                    if (flag_)
                        return true;
                    flag_ = true;
                    ::PostQueuedCompletionStatus(handle_, 0, 0, (LPOVERLAPPED)
                    this);
                }

            private:
                void doComplete(size_t bytes_transferred, std::error_code ec) override {
                    // ...
                    flag_ = false;
                }

            private:
                HANDLE handle_;
                volatile bool flag_{false};
            };
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP__WAKEUP_CHANNEL_H_