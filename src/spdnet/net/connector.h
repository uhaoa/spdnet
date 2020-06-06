#ifndef SPDNET_NET_CONNECTOR_H_
#define SPDNET_NET_CONNECTOR_H_

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
    namespace net {
        class EventService;

        class ConnectSession;

        class AsyncConnector : public base::NonCopyable {
        public:
            friend ConnectSession;
            using FailedCallback = std::function<void()>;

            AsyncConnector(EventService &service);

            ~AsyncConnector();

            void asyncConnect(const std::string &ip, int port, TcpSession::TcpEnterCallback &&success_cb,
                              FailedCallback &&failed_cb);

        private:
            bool removeSession(int fd);

        private:
            EventService &service_;
            std::mutex session_guard_;
            std::unordered_map<int, std::shared_ptr<ConnectSession>> connect_sessions_;
        };


    }
}
#endif  // SPDNET_NET_CONNECTOR_H_