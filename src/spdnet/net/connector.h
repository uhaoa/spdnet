#ifndef SPDNET_NET_CONNECTOR_H_
#define SPDNET_NET_CONNECTOR_H_
#include <mutex>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/platform.h>
#if defined(SPDNET_PLATFORM_LINUX)
#include <spdnet/net/detail/impl_linux/connector_impl.h>
#endif

namespace spdnet {
    namespace net {
        class EventService;
        class AsyncConnector : public base::NonCopyable {
        public:
#if defined(SPDNET_PLATFORM_LINUX)
            using ImplType = detail::AsyncConnectorImpl;
#endif

            AsyncConnector(EventService &service)
                    :impl_(service)
            {

            }

            ~AsyncConnector() = default;

            void asyncConnect(const EndPoint &addr, TcpSession::TcpEnterCallback &&success_cb,
                              std::function<void()> &&failed_cb)
            {
                impl_.asyncConnect(addr , std::move(success_cb) , std::move(failed_cb));
            }

        private:
            ImplType impl_;
        };


    }
}
#endif  // SPDNET_NET_CONNECTOR_H_