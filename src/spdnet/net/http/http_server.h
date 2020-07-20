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
                using http_enter_callback = std::function<void(std::shared_ptr<http_session>)>;
            public:
                http_server(spdnet::net::event_service &service);

                ~http_server();

                void start(const end_point &addr, http_session::http_request_callback&& enter_callback);

                //void stop();
            private:
                void remove_http_session(sock_t fd);
                void add_http_session(sock_t fd, std::shared_ptr<http_session>);
            private:
                spdnet::net::tcp_acceptor acceptor_;
                std::mutex mutex_;
                std::unordered_map<sock_t , std::shared_ptr<http_session>> http_sessions_;
            };
        }

    }
}

#include <spdnet/net/http/http_server.ipp>

#endif  // SPDNET_NET_HTTP_HTTP_SERVER_H_