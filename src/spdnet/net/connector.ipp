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
                : service_(service) {
            cancel_token_ = std::make_shared<std::atomic_bool>(false);
        }

        AsyncConnector::~AsyncConnector() {
            // lock
            while (cancel_token_->exchange(true));
            std::lock_guard<std::mutex> lck(context_guard_);
            for (auto &pair : connecting_context_) {
                socket_ops::closeSocket(pair.first);
                auto context = pair.second;
                auto service_thread = context->getServiceThread();
                service_thread->getChannelCollector()->putChannel(context);
            }
            connecting_context_.clear();
        }

        bool AsyncConnector::recycleContext(sock_t fd, std::shared_ptr<ServiceThread> service_thread) {
            std::lock_guard<std::mutex> lck(context_guard_);
            assert(connecting_context_.count(fd) > 0);
            service_thread->getChannelCollector()->putChannel(connecting_context_[fd]);
            return connecting_context_.erase(fd) > 0;
        }

        void AsyncConnector::asyncConnect(const EndPoint &addr, TcpEnterCallback &&enter_cb,
                                          std::function<void()> &&failed_cb) {
            sock_t client_fd = socket_ops::createSocket(addr.family(), SOCK_STREAM, 0);
            if (client_fd == invalid_socket)
                return;
            socket_ops::socketNonBlock(client_fd);

            auto service_thread = service_.getServiceThread();
            auto enter = std::move(enter_cb);
            auto failed = std::move(failed_cb);
            auto &this_ref = *this;
            /*
             * 当AsyncConnector析构时， 被success和failed lamba捕获的AsyncConnector引用也就失效了, 而IO线程还是可能在它析构后调用这个回调。
             * 这里使用一个shared_ptr的cancel_token_ ，主要是为了当io线程调用callback时通知这个AsyncConnector引用已经失效了 ，
             * 从而避免使用失效引用导至crash。
            **/
            auto cancel_token = cancel_token_;
            auto&& success_notify = [client_fd, enter, service_thread, &this_ref, cancel_token]() {
                // try lock
                if (cancel_token->exchange(true)) { return; }
                this_ref.service_.addTcpSession(client_fd, false, enter, service_thread);
                this_ref.recycleContext(client_fd, service_thread);
                cancel_token->exchange(false); 
            };
            auto&& failed_notify = [client_fd, failed, service_thread, &this_ref, cancel_token]() mutable {
                // try lock
                if (cancel_token->exchange(true)) { return; }
                this_ref.recycleContext(client_fd, service_thread);
                if (failed)
                    failed();
                socket_ops::closeSocket(client_fd);
                cancel_token->exchange(false);
            };
            auto context = std::make_shared<detail::ConnectContext>(client_fd, service_thread, std::move(success_notify),std::move(failed_notify));
            {
                std::lock_guard<std::mutex> lck(context_guard_);
                connecting_context_[client_fd] = context;
            }
            if (!service_thread->getImpl()->asyncConnect(client_fd, addr, context.get())) {
                recycleContext(client_fd, service_thread);
                socket_ops::closeSocket(client_fd);
            }
        }

    }
}
#endif // SPDNET_NET_CONNECTOR_IPP_