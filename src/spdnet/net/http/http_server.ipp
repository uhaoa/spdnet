#ifndef SPDNET_NET_HTTP_HTTP_SERVER_IPP_
#define SPDNET_NET_HTTP_HTTP_SERVER_IPP_

#include <memory>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/acceptor.h>
#include <spdnet/net/http/http_server.h>


namespace spdnet {
    namespace net {
        namespace http {
            http_server::http_server(event_service &service)
             :acceptor_(service)
            {

            }

            http_server::~http_server()
            {

            }

			void http_server::start(const end_point& addr, http_request_callback&& enter_cb)
            {
                acceptor_.start(addr , [this](std::shared_ptr<tcp_session> new_tcp_session){
                    auto new_http_session = std::make_shared<http_session>(new_tcp_session) ;
                    this->add_http_session(new_http_session);

                    new_tcp_session->set_data_callback([new_http_session](const char *data, size_t len) -> size_t {
                        return new_http_session->try_parse(data , len);
                    });
                    new_tcp_session->set_disconnect_callback([this](std::shared_ptr<tcp_session> session) {
                        this->remove_http_session(session->sock_fd());
                    });

					new_http_session->set_http_request_callback(enter_cb); 

                    new_tcp_session->set_no_delay();
                })
            }

            void http_server::add_http_session(sock_t fd, std::shared_ptr<http_session> session) {
                std::lock_guard<std::mutex> lck(mutex_);
                http_sessions_[fd] = std::move(session);
            }

            void http_server::remove_http_session(sock_t fd) {
                std::lock_guard<std::mutex> lck(mutex_);
                auto iter = http_sessions_.find(fd);
                if (iter != tcp_sessions_.end()) {
                    http_sessions_.erase(iter);
                } else {
                    assert(false);
                }
            }

        }
    }
}


#endif  // SPDNET_NET_HTTP_HTTP_SERVER_IPP_