#ifndef SPDNET_NET_CONNECTOR_IPP_
#define SPDNET_NET_CONNECTOR_IPP_

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
                   auto service_thread = context->getServiceThread(); 
                   service_thread->getExecutor()->post([context]() mutable {
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

				auto service_thread = service_.getServiceThread();
				auto enter = std::move(enter_cb);
				auto failed = std::move(failed_cb);
				auto fd = client_fd;
				auto& this_ref = *this;
				std::weak_ptr<char> cancel_token = cancel_token_;
				auto success_notify = [fd, enter, service_thread, cancel_token, &this_ref]() {
					auto share_token = cancel_token.lock();
					if (share_token)
						this_ref.service_.addTcpSession(fd, false, enter);
					service_thread->getExecutor()->post([fd, cancel_token, &this_ref]() mutable {
						auto token = cancel_token.lock();
						if (token)
							this_ref.removeContext(fd);
						});
				};
				auto failed_notify = [fd, failed, service_thread, cancel_token, &this_ref]() mutable {
					service_thread->getExecutor()->post([fd, cancel_token, &this_ref]() {
						auto token = cancel_token.lock();
						if (token)
							this_ref.removeContext(fd);
						});
					if (failed)
						failed();
				};
				auto context = std::make_shared<detail::ConnectContext>(client_fd, service_thread, success_notify, failed_notify);
				{
					std::lock_guard<std::mutex> lck(context_guard_);
					connecting_context_[client_fd] = context;
				}
                if (!service_thread->getImpl()->asyncConnect(client_fd, addr, context.get())){
                    removeContext(client_fd);
                }
            }

    }
}
#endif // SPDNET_NET_CONNECTOR_IPP_