#ifndef SPDNET_NET_EPOLL_CHANNEL_H_
#define SPDNET_NET_EPOLL_CHANNEL_H_
#include <spdnet/base/platform.h>
#include <iostream>

namespace spdnet {
    namespace net {
        class tcp_session;
        namespace detail {
            class epoll_impl;
            class channel
            {
            public:
                channel() = default;

				virtual ~channel() noexcept {}

				virtual void on_send() = 0;

				virtual void on_recv() = 0;

				virtual void on_close() = 0;
            };

			class socket_channel : public channel {
			public:
				socket_channel(std::shared_ptr<tcp_session> session, std::shared_ptr<epoll_impl> io_impl)
					: session_(session), io_impl_(io_impl)
				{

				}
			protected:
				std::shared_ptr<tcp_session> session_;
				std::shared_ptr<epoll_impl> io_impl_;
			};

        }
    }

}

#endif  // SPDNET_NET_EPOLL_CHANNEL_H_