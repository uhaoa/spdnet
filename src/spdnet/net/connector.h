#ifndef SPDNET_NET_CONNECTOR_H_
#define SPDNET_NET_CONNECTOR_H_
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
        class ConnectContext;
#if defined(SPDNET_PLATFORM_LINUX)
        namespace detail
        {
            class ConnectorChannel；
        }
#endif
        class AsyncConnector : public base::NonCopyable {
        public:
#if defined(SPDNET_PLATFORM_LINUX)
            friend class detail::ConnectorChannel;
#endif
            using FailedCallback = std::function<void()>;

            AsyncConnector(EventService &service);

            ~AsyncConnector();

            void asyncConnect(const EndPoint &addr, TcpSession::TcpEnterCallback &&success_cb,
                              FailedCallback &&failed_cb);

        private:
            bool removeSession(int fd);
            void onSuccess(int fd);
            void onFailed(int fd);
        private:
            EventService &service_;
            std::mutex session_guard_;
            std::unordered_map<int, std::shared_ptr<ConnectContext>> connecting_sessions_;
        };


    }
}
#endif  // SPDNET_NET_CONNECTOR_H_