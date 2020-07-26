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
                http_connector(spdnet::net::event_service &service)
                        : connector_(service) {

                }

                ~http_connector() = default;

                void async_connect(const end_point &addr, http_session::http_enter_callback &&enter_callback,
                                   async_connector::connect_failed_callback &&failed_callback) {
                    auto &&callback = std::move(enter_callback);
                    connector_.async_connect(addr,
                                             [callback](std::shared_ptr<spdnet::net::tcp_session> new_tcp_session) {
                                                 auto new_http_session = std::make_shared<http_session>(new_tcp_session,
                                                                                                        false);

                                                 new_tcp_session->set_data_callback(
                                                         [new_http_session](const char *data, size_t len) -> size_t {
                                                             assert(new_http_session != nullptr);
                                                             return new_http_session->try_parse(data, len);
                                                         });

                                                 new_tcp_session->set_disconnect_callback(
                                                         [new_http_session](std::shared_ptr<tcp_session> session) {
                                                             new_http_session->reset();
                                                         });

                                                 if (callback)
                                                     callback(new_http_session);
                                             }, std::move(failed_callback));
                }

            private:
                spdnet::net::async_connector connector_;
            };
        }

    }
}

#endif  // SPDNET_NET_HTTP_HTTP_CONNECTOR_H_