#ifndef SPDNET_NET_HTTP_HTTP_SESSION_H_
#define SPDNET_NET_HTTP_HTTP_SESSION_H_

#include <memory>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/http/http_parser.h>
#include <spdnet/net/http/http_parser_api.h>
#include <spdnet/base/SHA1.hpp>
#include <spdnet/base/base64.h>

namespace spdnet {
    namespace net {
        namespace http {
            class http_session : public spdnet::base::noncopyable, public std::enable_shared_from_this<http_session> {
            public:
				using http_enter_callback = std::function<void(std::shared_ptr<http_session>)>;
				using http_request_callback = std::function<void(const http_request& , std::shared_ptr<http_session>)>;
				using http_response_callback = std::function<void(const http_response&, std::shared_ptr<http_session>)>;
                using ws_frame_enter_callback = std::function<void(const websocket_frame&, std::shared_ptr<http_session>)>;
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
						// throw ? 
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
						// throw ?;
						return;
					}
					auto this_ptr = shared_from_this();
					response_parser_.set_parse_complete_callback([callback, this_ptr](const http_response& response) {
						callback(response, this_ptr);
					});
				}

				void set_ws_frame_enter_callback(const ws_frame_enter_callback& callback)
                {
                    auto this_ptr = shared_from_this();
                    request_parser_.set_ws_frame_complete_callback([callback, this_ptr](const websocket_frame& packet) {
                        if (packet.get_opcode() == ws_opcode::op_handshake_req){
                            std::string sec_key = packet.ws_key_;
                            sec_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

                            CSHA1 s1;
                            s1.Update((uint8_t *)sec_key.c_str(), static_cast<uint32_t>(sec_key.size()));
                            s1.Final();
                            uint8_t hash_buf[20] = {0};
                            s1.GetHash(hash_buf);

                            std::string base64_str = spdnet::base::util::base64_encode((const unsigned char*)hash_buf, sizeof(hash_buf));

                            std::string handshake_ack = "HTTP/1.1 101 Switching Protocols\r\n"
                                                   "Upgrade: websocket\r\n"
                                                   "Connection: Upgrade\r\n"
                                                   "Sec-WebSocket-Accept: ";

                            handshake_ack += base64_str;
                            handshake_ack += "\r\n\r\n";

                            this_ptr->session_->send(handshake_ack.c_str() ,handshake_ack.length() , [this_ptr](){
                                // call ws enter callback
                            });


                        }
                        else if (packet.get_opcode() == ws_opcode::op_handshake_ack)
                        {
                            //  // call ws enter callback
                        }
                        else {
                            // user callback
                            callback(packet, this_ptr);
                        }
                    });
                }
				void send_response(const http_response& resp)
				{
					assert(is_server_side_);
					auto stream = resp.to_string();
					auto this_ptr = shared_from_this();
					if (session_)
						session_->send(stream.c_str(), stream.length() , [this_ptr]() {
						if (!this_ptr->request_parser_.get_request().is_keep_alive()) {
							this_ptr->shutdown(); 
						}
						});
					else
					{
						// throw error ?; 
					}
				}

				void send_request(const http_request& req)
				{
					assert(!is_server_side_);
					auto stream = req.to_string();
					if (session_)
						session_->send(stream.c_str(), stream.length());
					else
					{
						// throw error ?; 
					}
				}

				void reset()
				{
					session_ = nullptr; 
				}

				void shutdown()
				{
					session_->post_shutdown(); 
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
                websocket_parser ws_parser_;
            };
        }
    }
}

#endif // SPDNET_NET_HTTP_HTTP_SESSION_H_