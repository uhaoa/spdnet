#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP_CHANNEL_H_
#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/io_type_define.h>

namespace spdnet {
    namespace net {
		class TcpSocketData;
        namespace detail {
            class Channel : public OVERLAPPED
            {
            public:
				Channel() {
                    reset();
                }
				virtual ~Channel(){}

                virtual void doComplete(size_t bytes_transferred , std::error_code ec) = 0;

                void reset() {
					Internal = 0;
					InternalHigh = 0;
					Offset = 0;
					OffsetHigh = 0;
					hEvent = 0;
                }
            };
            class SocketChannel : public Channel {
            public:
				SocketChannel(TcpSocketData& data, std::shared_ptr<IoObjectImplType> io_impl)
                    :socket_data_(data), io_impl_(io_impl)
                {

                }
                virtual ~SocketChannel() noexcept {}
            protected:
				TcpSocketData& socket_data_;
                std::shared_ptr<IoObjectImplType> io_impl_;
            };

        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP_CHANNEL_H_