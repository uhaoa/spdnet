#ifndef SPDNET_NET_HTTP_HTTP_CONNECTOR_H_
#define SPDNET_NET_HTTP_HTTP_CONNECTOR_H_

#include <memory>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/end_point.h>
#include <spdnet/net/http/http_session.h>
#include <spdnet/net/connector.h>

namespace spdnet {
    namespace net {
        namespace http {
            class http_connector : public spdnet::base::noncopyable {
            public:
				http_connector(spdnet::net::event_service &service);

				~http_connector() = default; 

                void async_connect(const end_point &addr, http_session::http_enter_callback&& enter_callback , async_connector::connect_failed_callback&& failed_callback);

                //void stop();
            private:
               // void remove_http_session(sock_t fd);
               // void add_http_session(sock_t fd, std::shared_ptr<http_session>);
            private:
				spdnet::net::async_connector connector_;
            };
        }

    }
}

#include <spdnet/net/http/http_connector.ipp>

#endif  // SPDNET_NET_HTTP_HTTP_CONNECTOR_H_