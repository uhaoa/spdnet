#ifndef SPDNET_NET_ACCEPTOR_IPP_
#define SPDNET_NET_ACCEPTOR_IPP_

#include <spdnet/net/acceptor.h>
#include <iostream>
#include <algorithm>
#include <spdnet/net/socket_ops.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/event_service.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/platform.h>

namespace spdnet {
    namespace net {

        tcp_acceptor::tcp_acceptor(event_service &service)
                : service_(service) {
        }

        tcp_acceptor::~tcp_acceptor() {
            if (listen_thread_) {
                auto collector = listen_thread_->get_channel_collector();
                assert(collector);
                collector->put_channel(accept_channel_);
            }
            stop();
        }

        void tcp_acceptor::start(const end_point &addr, tcp_enter_callback &&enter_cb) {
            addr_ = addr;
            listen_fd_ = create_listen_socket(addr);
            if (listen_fd_ == invalid_socket) {
                throw spdnet_exception(std::string("listen error : ") + std::to_string(current_errno()));
            }
            socket_ops::socket_non_block(listen_fd_);
            auto &service = service_;
#if defined(SPDNET_PLATFORM_WINDOWS)
            accept_channel_ = std::make_shared<detail::accept_channel_impl>(listen_fd_, addr, [&service, enter_cb](sock_t new_socket) {
                service.add_tcp_session(new_socket, true, enter_cb);

                });
#else
            accept_channel_ = std::make_shared<detail::accept_channel_impl>(listen_fd_,
                                                                          [&service, enter_cb](sock_t new_socket) {
                                                                              service.add_tcp_session(new_socket, true,
                                                                                                    enter_cb);
                                                                          });
#endif
            run_listen_ = std::make_shared<bool>(true);
            listen_thread_ = std::make_shared<service_thread>(default_loop_timeout);/*service_.service_thread();*/
            listen_thread_->run(run_listen_);
            if (!listen_thread_->get_impl()->start_accept(listen_fd_, accept_channel_.get())) {
                throw spdnet_exception(std::string("listen error : ") + std::to_string(current_errno()));
            }
        }

        void tcp_acceptor::stop() {
            try {
                if (*run_listen_) {
                    *run_listen_ = false;
                    if (listen_thread_->get_thread()->joinable())
                        listen_thread_->get_thread()->join();

                    listen_thread_ = nullptr;
                }
            }
            catch (std::system_error &err) {
                (void) err;
            }

        }

        sock_t tcp_acceptor::create_listen_socket(const end_point &addr) {
            sock_t fd = ::socket(addr.family(), SOCK_STREAM, 0);
            if (fd == invalid_socket) {
                return invalid_socket;
            }

            int reuse_on = 1;
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuse_on, sizeof(int)) < 0) {
                socket_ops::close_socket(fd);
                return invalid_socket;
            }

            int ret = ::bind(fd, addr.socket_addr(), addr.socket_addr_len());
            if (ret == SPDNET_SOCKET_ERROR || listen(fd, 512) == SPDNET_SOCKET_ERROR) {
                socket_ops::close_socket(fd);
                return invalid_socket;
            }

            return fd;

        }
    }
}

#endif // SPDNET_NET_ACCEPTOR_IPP_