#ifndef SPDNET_NET_CONNECTOR_H_
#define SPDNET_NET_CONNECTOR_H_

#include <mutex>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/service_thread.h>
#if defined(SPDNET_PLATFORM_LINUX)
#include <spdnet/net/detail/impl_linux/connect_context.h>
#else
#include <spdnet/net/detail/impl_win/connect_context.h>
#endif

namespace spdnet {
    namespace net {
        class EventService;

        class SPDNET_EXPORT AsyncConnector : public base::NonCopyable {
        public:
            AsyncConnector(EventService& service); 
            ~AsyncConnector();

            void asyncConnect(const EndPoint& addr, TcpEnterCallback&& success_cb, std::function<void()>&& failed_cb);
        private:
            bool removeContext(sock_t fd);
        private:
            EventService& service_;
            std::shared_ptr<char> cancel_token_; 
			std::mutex context_guard_;
			std::unordered_map<sock_t, std::shared_ptr<detail::ConnectContext>> connecting_context_;
        };


    }
}
#endif  // SPDNET_NET_CONNECTOR_H_