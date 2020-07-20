#ifndef SPDNET_NET_HTTP_HTTP_SESSION_H_
#define SPDNET_NET_HTTP_HTTP_SESSION_H_

#include <memory>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/http/http_parser.h>
#include <spdnet/net/http/http_parser_api.h>
namespace spdnet {
    namespace net {
        namespace http {
            class http_session : public spdnet::base::noncopyable, public std::enable_shared_from_this<http_session> {
            public:
				using http_request_callback = std::function<void(const http_request& , std::shared_ptr<http_session>)>;
				friend class http_server; 
				http_session(std::shared_ptr<tcp_session> session)
					:session_(session)
				{
				}

				~http_session() = default;

				void set_http_request_callback(const http_request_callback& callback)
				{
					auto this_ptr = shared_from_this();
					parser_.set_parse_complete_callback([callback , this_ptr](const http_request& request) {
						callback(request , this_ptr);
					});
				}
            private:
				size_t try_parse(const char* data, size_t len)
				{
					return parser_.try_parse(data, len);
				}
            private:
                std::shared_ptr<tcp_session> session_;
				http_request_parser parser_; 
            };
        }
    }
}

#endif // SPDNET_NET_HTTP_HTTP_SESSION_H_