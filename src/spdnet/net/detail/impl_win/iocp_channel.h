#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP_CHANNEL_H_

#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/socket_data.h>

namespace spdnet {
    namespace net {
        namespace detail {
            // class IocpSocketData;
            class iocp_impl;

            class channel : public OVERLAPPED {
            public:
                channel() {
                    reset();
                }

                virtual ~channel() {}

                virtual void do_complete(size_t bytes_transferred, std::error_code ec) = 0;

                void reset() {
                    Internal = 0;
                    InternalHigh = 0;
                    Offset = 0;
                    OffsetHigh = 0;
                    hEvent = 0;
                }
            };

            class socket_channel : public channel {
            public:
                socket_channel(socket_data::ptr data, std::shared_ptr<iocp_impl> io_impl)
                        : data_(data), io_impl_(io_impl) {

                }

                virtual ~socket_channel() noexcept {}

            protected:
                socket_data::ptr data_;
                std::shared_ptr<iocp_impl> io_impl_;
            };

        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP_CHANNEL_H_