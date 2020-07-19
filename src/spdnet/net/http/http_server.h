#ifndef SPDNET_NET_HTTP_HTTP_SERVER_H_
#define SPDNET_NET_HTTP_HTTP_SERVER_H_

#include <memory>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/end_point.h>

namespace spdnet {
    namespace net {
        namespace http {
            class event_service;
            class http_server : public spdnet::base::noncopyable {
            public:
                using http_enter_callback = std::function<void(std::shared_ptr<http_session>)>;
            public:
                http_server(event_service &service);

                ~http_server();

                void start(const end_point &addr, http_enter_callback &&enter_cb);

                //void stop();
            private:
                void remove_http_session(sock_t fd);
                void add_http_session(sock_t fd, std::shared_ptr<HttpSession>);
            private:
                spdnet::net::tcp_acceptor acceptor_;
                std::mutex mux_;
                std::unordered_map<sock_t , std::shared_ptr<HttpSession>> http_sessions_;
            };
        }

    }
}

#include <spdnet/net/http/http_server.ipp>

#endif  // SPDNET_NET_HTTP_HTTP_SERVER_H_