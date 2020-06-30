#include <spdnet/net/connector.h>
#include <unordered_map>
#include <chrono>
#include <set>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <spdnet/net/socket_ops.h>
#include <spdnet/net/event_service.h>

namespace spdnet {
    namespace net {
            AsyncConnector::AsyncConnector(EventService &service)
                    : service_(service) 
            {
                cancel_token_ = std::make_shared<char>();
            }

            AsyncConnector::~AsyncConnector()
            {
               std::lock_guard<std::mutex> lck(context_guard_);
               for (auto& pair : connecting_context_)
               {
                   auto context = pair.second; 
                   auto loop = context->getLoop(); 
				   loop->post([context]() mutable {
                       context = nullptr;
                   });
               }
               connecting_context_.clear();
            }

            bool AsyncConnector::removeContext(sock_t fd) {
                std::lock_guard<std::mutex> lck(context_guard_);
                return connecting_context_.erase(fd) > 0;
            }

            void AsyncConnector::asyncConnect(const EndPoint& addr, TcpEnterCallback&& enter_cb,
                std::function<void()>&& failed_cb) {
                sock_t client_fd = socket_ops::createSocket(addr.family(), SOCK_STREAM, 0);
                if (client_fd == invalid_socket)
                    return;
                socket_ops::socketNonBlock(client_fd);
				/*
                int ret = ::connect(client_fd, addr.socket_addr(), addr.socket_addr_len());
                if (ret == 0) {
                    if (socket_ops::checkSelfConnect(client_fd))
                        goto FAILED;

                    service_.addTcpSession(client_fd, enter_cb);
                }
#if defined(SPDNET_PLATFORM_WINDOWS)
                else if (current_errno() != WSAEWOULDBLOCK) {
                    goto FAILED;
                }
#else
				else if (current_errno() != EINPROGRESS) {
					goto FAILED;
				}
#endif
*/
                /*else*/ {
                    auto loop = service_.getEventLoop();
                    auto enter = std::move(enter_cb);
                    auto failed = std::move(failed_cb);
                    auto fd = client_fd;
                    auto& this_ref = *this;
                    std::weak_ptr<char> cancel_token = cancel_token_;
                    auto success_notify = [fd, enter, loop, cancel_token, &this_ref]() {
                        auto share_token = cancel_token.lock();
                        if (share_token)
                            this_ref.service_.addTcpSession(fd, false , enter);
						loop->post([fd, cancel_token, &this_ref]() mutable {
							auto token = cancel_token.lock();
							if (token)
								this_ref.removeContext(fd);
							});
                    };
                    auto failed_notify = [fd, failed, loop, cancel_token, &this_ref]() mutable {
                        loop->post([fd , cancel_token  , &this_ref]() {
                            auto token = cancel_token.lock();
                            if (token)
                                this_ref.removeContext(fd);
                            this_ref.removeContext(fd);
                            });
                        if (failed)
                            failed();
                    };
                    auto context = std::make_shared<detail::ConnectContext>(client_fd, loop, success_notify, failed_notify);
                    {
                        std::lock_guard<std::mutex> lck(context_guard_);
                        connecting_context_[client_fd] = context;
                    }
					context->reset(); 
                    loop->getImplRef().asyncConnect(client_fd, addr, context.get());
                    return;

                }

            FAILED:
                if (client_fd != -1) {
                    socket_ops::closeSocket(client_fd);
                    client_fd = invalid_socket;
                }
                if (failed_cb != nullptr) {
                    failed_cb();
                }

            }

    }
}