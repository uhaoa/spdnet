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
				using http_enter_callback = std::function<void(std::shared_ptr<http_session>)>;
				using http_request_callback = std::function<void(const http_request& , std::shared_ptr<http_session>)>;
				using http_response_callback = std::function<void(const http_response&, std::shared_ptr<http_session>)>;
				friend class http_server; 
				friend class http_connector;

				http_session(std::shared_ptr<tcp_session> session , bool is_server_side)
					:is_server_side_(is_server_side) , session_(session)
				{
				}
				~http_session() = default;


				void set_http_request_callback(const http_request_callback& callback)
				{
					if (!is_server_side_) {
						// throw;
						return;
					}
					auto this_ptr = shared_from_this();
					request_parser_.set_parse_complete_callback([callback , this_ptr](const http_request& request) {
						callback(request , this_ptr);
					});
				}

				void set_http_response_callback(const http_response_callback& callback)
				{
					if (is_server_side_) {
						// throw;
						return;
					}
					auto this_ptr = shared_from_this();
					response_parser_.set_parse_complete_callback([callback, this_ptr](const http_response& response) {
						callback(response, this_ptr);
					});
				}

				void send_response(http_response& resp)
				{
					auto stream = resp.to_string();
					if (session_)
						session_->send(stream.c_str() , stream.length());
					else
					{
						// throw error; 
					}
				}

				void send_request(http_request& req)
				{
					auto stream = req.to_string();
					if (session_)
						session_->send(stream.c_str(), stream.length());
					else
					{
						// throw error; 
					}
				}

				void reset()
				{
					session_ = nullptr; 
				}
            private:
				size_t try_parse(const char* data, size_t len)
				{
					if (is_server_side_)
						return request_parser_.try_parse(data, len);
					else 
						return response_parser_.try_parse(data, len);
				}
            private:
				bool is_server_side_{ false };
                std::shared_ptr<tcp_session> session_;
				http_request_parser request_parser_; 
				http_response_parser response_parser_;
            };
        }
    }
}

#endif // SPDNET_NET_HTTP_HTTP_SESSION_H_