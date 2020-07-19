#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP__WAKEUP_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP__WAKEUP_CHANNEL_H_

#include <system_error>
#include <spdnet/net/detail/impl_win/iocp_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class iocp_wakeup_channel : public channel {
            public:
                iocp_wakeup_channel(HANDLE handle)
                        : handle_(handle) {

                }

                void wakeup() {
                    ::PostQueuedCompletionStatus(handle_, 0, 0, (LPOVERLAPPED)
                    this);
                }

            private:
                void do_complete(size_t bytes_transferred, std::error_code ec) override {
                    // ...
                }

            private:
                HANDLE handle_;
            };
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP__WAKEUP_CHANNEL_H_