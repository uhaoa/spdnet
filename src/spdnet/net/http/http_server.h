#ifndef SPDNET_NET_HTTP_HTTP_SERVER_H_
#define SPDNET_NET_HTTP_HTTP_SERVER_H_

#include <memory>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/end_point.h>
#include <spdnet/net/http/http_session.h>
#include <spdnet/net/acceptor.h>

namespace spdnet {
    namespace net {
        namespace http {
            class http_server : public spdnet::base::noncopyable {
            public:
                http_server(spdnet::net::event_service &service);

				~http_server() = default; 

                void start(const end_point &addr, http_session::http_enter_callback&& enter_callback);
            private:
                spdnet::net::tcp_acceptor acceptor_;
            };
        }

    }
}

#include <spdnet/net/http/http_server.ipp>

#endif  // SPDNET_NET_HTTP_HTTP_SERVER_H_