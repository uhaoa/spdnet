#ifndef SPDNET_NET_CONNECTOR_IMPL_H_
#define SPDNET_NET_CONNECTOR_IMPL_H_
#include <mutex>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/platform.h>

namespace spdnet {
    namespace net {
        class EventService;
        namespace detail {
            class ConnectContext;
            class AsyncConnectorImpl : public base::NonCopyable {
            public:
                AsyncConnectorImpl(EventService &service);

                ~AsyncConnectorImpl() = default;

                void asyncConnect(const EndPoint &addr, TcpSession::TcpEnterCallback &&success_cb,
                                  std::function<void()> &&failed_cb);

            private:
                bool removeContext(int fd);

            private:
                EventService &service_;
                std::mutex context_guard_;
                std::unordered_map<int, std::shared_ptr<ConnectContext>> connecting_context_;
            };

        }
    }
}
#endif  // SPDNET_NET_CONNECTOR_IMPL_H_