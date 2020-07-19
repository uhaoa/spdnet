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
        async_connector::async_connector(event_service &service)
                : service_(service) {
            cancel_token_ = std::make_shared<std::atomic_bool>(false);
        }

        async_connector::~async_connector() {
            // lock
            while (cancel_token_->exchange(true));
            std::lock_guard<std::mutex> lck(context_guard_);
            for (auto &pair : connecting_context_) {
                socket_ops::close_socket(pair.first);
                auto context = pair.second;
                auto thread = context->get_service_thread();
                thread->get_channel_collector()->put_channel(context);
            }
            connecting_context_.clear();
        }

        bool async_connector::recycle_context(sock_t fd, std::shared_ptr<service_thread> thread) {
            std::lock_guard<std::mutex> lck(context_guard_);
            assert(connecting_context_.count(fd) > 0);
            thread->get_channel_collector()->put_channel(connecting_context_[fd]);
            return connecting_context_.erase(fd) > 0;
        }

        void async_connector::async_connect(const end_point &addr, tcp_enter_callback &&enter_cb,
                                          std::function<void()> &&failed_cb) {
            sock_t client_fd = socket_ops::create_socket(addr.family(), SOCK_STREAM, 0);
            if (client_fd == invalid_socket)
                return;
            socket_ops::socket_non_block(client_fd);

            auto thread = service_.get_service_thread();
            auto enter = std::move(enter_cb);
            auto failed = std::move(failed_cb);
            auto &this_ref = *this;
            /*
             * 当async_connector析构时， 被success和failed lamba捕获的async_connector引用也就失效了, 而IO线程还是可能在它析构后调用这个回调。
             * 这里使用一个shared_ptr的cancel_token_ ，主要是为了当io线程调用callback时通知这个async_connector引用已经失效了 ，
             * 从而避免使用失效引用导至crash。
            **/
            auto cancel_token = cancel_token_;
            auto&& success_notify = [client_fd, enter, thread, &this_ref, cancel_token]() {
                // try lock
                if (cancel_token->exchange(true)) { return; }
                this_ref.service_.add_tcp_session(client_fd, false, enter, thread);
                this_ref.recycle_context(client_fd, thread);
                cancel_token->exchange(false); 
            };
            auto&& failed_notify = [client_fd, failed, thread, &this_ref, cancel_token]() mutable {
                // try lock
                if (cancel_token->exchange(true)) { return; }
                this_ref.recycle_context(client_fd, thread);
                if (failed)
                    failed();
                socket_ops::close_socket(client_fd);
                cancel_token->exchange(false);
            };
            auto context = std::make_shared<detail::connect_context>(client_fd, thread, std::move(success_notify),std::move(failed_notify));
            {
                std::lock_guard<std::mutex> lck(context_guard_);
                connecting_context_[client_fd] = context;
            }
            if (!thread->get_impl()->async_connect(client_fd, addr, context.get())) {
                recycle_context(client_fd, thread);
                socket_ops::close_socket(client_fd);
            }
        }

    }
}
#endif // SPDNET_NET_CONNECTOR_IPP_