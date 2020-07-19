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
        class event_service;

        class async_connector : public base::noncopyable {
        public:
            async_connector(event_service &service);

            ~async_connector();

            void async_connect(const end_point &addr, tcp_enter_callback &&success_cb, std::function<void()> &&failed_cb);

        private:
            bool recycle_context(sock_t fd, std::shared_ptr<service_thread> service_thread);

        private:
            event_service &service_;
            std::mutex context_guard_;
            std::shared_ptr<std::atomic_bool> cancel_token_;
            std::unordered_map<sock_t, std::shared_ptr<detail::connect_context>> connecting_context_;
        };


    }
}

#include <spdnet/net/connector.ipp>

#endif  // SPDNET_NET_CONNECTOR_H_