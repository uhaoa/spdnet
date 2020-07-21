#ifndef SPDNET_NET_HTTP_HTTP_CONNECTOR_IPP_
#define SPDNET_NET_HTTP_HTTP_CONNECTOR_IPP_

#include <memory>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/acceptor.h>
#include <spdnet/net/http/http_connector.h>


namespace spdnet {
	namespace net {
		namespace http {
			http_connector::http_connector(spdnet::net::event_service& service)
				:connector_(service)
			{

			}

			http_connector::~http_connector()
			{

			}

			void http_connector::async_connect(const end_point& addr, http_session::http_enter_callback&& enter_callback)
			{
				auto&& callback = std::move(enter_callback);
				connector_.async_connect(addr, [callback](std::shared_ptr<spdnet::net::tcp_session> new_tcp_session) {
					auto new_http_session = std::make_shared<http_session>(new_tcp_session , false);

					new_tcp_session->set_data_callback([new_http_session](const char* data, size_t len) -> size_t {
						assert(new_http_session != nullptr); 
						return new_http_session->try_parse(data, len);
					});

					new_tcp_session->set_disconnect_callback([new_http_session](std::shared_ptr<tcp_session> session) {
						new_http_session->reset();
					});

					if (callback)
						callback(new_http_session);
					}, nullptr);
			}

		}
	}
}


#endif  // SPDNET_NET_HTTP_HTTP_CONNECTOR_IPP_