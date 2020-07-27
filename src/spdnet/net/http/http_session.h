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
                using http_request_callback = std::function<void(const http_request &, std::shared_ptr<http_session>)>;
                using http_response_callback = std::function<void(const http_response &,
                                                                  std::shared_ptr<http_session>)>;
                using ws_frame_enter_callback = std::function<void(const websocket_frame &,
                                                                   std::shared_ptr<http_session>)>;
                using ws_handshake_success_callback = std::function<void()>;

                friend class http_server;

                friend class http_connector;

                http_session(std::shared_ptr<tcp_session> session, bool is_server_side)
                        : session_(session), is_server_side_(is_server_side), request_parser_(ws_parser_),
                          response_parser_(ws_parser_) {
                }

                ~http_session() = default;


                void set_http_request_callback(const http_request_callback &callback) {
                    if (!is_server_side_) {
                        // throw ?
                        return;
                    }
                    auto this_ptr = shared_from_this();
                    request_parser_.set_parse_complete_callback([callback, this_ptr](const http_request &request) {
                        callback(request, this_ptr);
                    });
                }

                void set_http_response_callback(const http_response_callback &callback) {
                    if (is_server_side_) {
                        // throw ?;
                        return;
                    }
                    auto this_ptr = shared_from_this();
                    response_parser_.set_parse_complete_callback([callback, this_ptr](const http_response &response) {
                        callback(response, this_ptr);
                    });
                }

                void set_ws_handshake_success_callback(ws_handshake_success_callback &&callback) {
                    handshake_success_callback_ = std::move(callback);
                }

                void set_ws_frame_enter_callback(const ws_frame_enter_callback &callback) {
                    auto this_ptr = shared_from_this();
                    ws_parser_.set_ws_frame_complete_callback([callback, this_ptr](websocket_frame &frame) {
                        if (frame.get_opcode() == ws_opcode::op_handshake_req) {
                            std::string sec_key = frame.ws_key_;
                            sec_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

                            CSHA1 s1;
                            s1.Update((uint8_t *) sec_key.c_str(), static_cast<uint32_t>(sec_key.size()));
                            s1.Final();
                            uint8_t hash_buf[20] = {0};
                            s1.GetHash(hash_buf);

                            std::string base64_str = spdnet::base::util::base64_encode((const unsigned char *) hash_buf,
                                                                                       sizeof(hash_buf));

                            std::string handshake_ack = "HTTP/1.1 101 Switching Protocols\r\n"
                                                        "Upgrade: websocket\r\n"
                                                        "Connection: Upgrade\r\n"
                                                        "Sec-WebSocket-Accept: ";

                            handshake_ack += base64_str;
                            handshake_ack += "\r\n\r\n";

                            this_ptr->session_->send(handshake_ack.c_str(), handshake_ack.length(), [this_ptr]() {
                                if (this_ptr->handshake_success_callback_ != nullptr)
                                    this_ptr->handshake_success_callback_();
                            });
                            frame.reset();

                        } else if (frame.get_opcode() == ws_opcode::op_handshake_ack) {
                            if (this_ptr->handshake_success_callback_ != nullptr)
                                this_ptr->handshake_success_callback_();
                            frame.reset();
                        } else {
                            // user callback
                            callback(frame, this_ptr);
                        }
                    });
                }

                void send_response(const http_response &resp) {
                    assert(is_server_side_);
                    auto stream = resp.to_string();
                    auto this_ptr = shared_from_this();
                    if (session_)
                        session_->send(stream.c_str(), stream.length(), [this_ptr]() {
                            if (!this_ptr->request_parser_.get_request().is_keep_alive()) {
                                this_ptr->shutdown();
                            }
                        });
                    else {
                        // throw error ?;
                    }
                }

                void send_request(const http_request &req) {
                    assert(!is_server_side_);
                    auto stream = req.to_string();
                    if (session_)
                        session_->send(stream.c_str(), stream.length());
                    else {
                        // throw error ?;
                    }
                }

                void send_ws_frame(const websocket_frame &frame) {
                    auto stream = frame.to_string();
                    if (session_)
                        session_->send(stream.c_str(), stream.length());
                    else {
                        // throw ?
                    }
                }

                void start_ws_handshake(const std::string &host = "") {
                    http_request req;
                    req.set_url("/ws");
                    req.add_header("Host", host);
                    req.add_header("Upgrade", "websocket");
                    req.add_header("Connection", "Upgrade");
                    req.add_header("Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==");
                    req.add_header("Sec-WebSocket-Version", "13");

                    send_request(req);
                }

                void reset() {
                    session_ = nullptr;
                }

                void shutdown() {
                    session_->post_shutdown();
                }

                std::shared_ptr<tcp_session> under_tcp_session() {
                    return session_;
                }

            private:
                size_t try_parse(const char *data, size_t len) {
                    if (is_server_side_)
                        return request_parser_.try_parse(data, len);
                    else
                        return response_parser_.try_parse(data, len);
                }

            private:
                std::shared_ptr<tcp_session> session_;
                bool is_server_side_{false};
                websocket_parser ws_parser_;
                http_request_parser request_parser_;
                http_response_parser response_parser_;
                ws_handshake_success_callback handshake_success_callback_;
            };
        }
    }
}

#endif // SPDNET_NET_HTTP_HTTP_SESSION_H_