#ifndef SPDNET_NET_HTTP_HTTP_SERVER_IPP_
#define SPDNET_NET_HTTP_HTTP_SERVER_IPP_

#include <memory>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/acceptor.h>
#include <spdnet/net/http/http_server.h>


namespace spdnet {
    namespace net {
        namespace http {
            http_server::http_server(spdnet::net::event_service &service)
             :acceptor_(service)
            {

            }

            http_server::~http_server()
            {

            }

			void http_server::start(const end_point& addr, http_session::http_enter_callback&& enter_callback)
            {
                auto&& callback = std::move(enter_callback);
                acceptor_.start(addr , [this , callback](std::shared_ptr<tcp_session> new_tcp_session){
					auto new_http_session = std::make_shared<http_session>(new_tcp_session, true);
                    new_tcp_session->set_data_callback([new_http_session](const char *data, size_t len) -> size_t {
                        return new_http_session->try_parse(data , len);
                    });
                    new_tcp_session->set_disconnect_callback([new_tcp_session](std::shared_ptr<tcp_session> session) {

                    });

					if (callback)
						callback(new_http_session);
                });
            }

        }
    }
}


#endif  // SPDNET_NET_HTTP_HTTP_SERVER_IPP_