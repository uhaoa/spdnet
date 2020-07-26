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
                http_server(spdnet::net::event_service &service)
                        : acceptor_(service) {

                }

                ~http_server() = default;

                void start(const end_point &addr, http_session::http_enter_callback &&enter_callback) {
                    auto &&callback = std::move(enter_callback);
                    acceptor_.start(addr, [this, callback](std::shared_ptr<tcp_session> new_tcp_session) {
                        auto new_http_session = std::make_shared<http_session>(new_tcp_session, true);
                        new_tcp_session->set_data_callback([new_http_session](const char *data, size_t len) -> size_t {
                            return new_http_session->try_parse(data, len);
                        });
                        new_tcp_session->set_disconnect_callback(
                                [new_tcp_session](std::shared_ptr<tcp_session> session) {

                                });

                        if (callback)
                            callback(new_http_session);
                    });
                }

            private:
                spdnet::net::tcp_acceptor acceptor_;
            };
        }

    }
}

#endif  // SPDNET_NET_HTTP_HTTP_SERVER_H_