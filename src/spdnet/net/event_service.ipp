#ifndef SPDNET_NET_EVENTSERVICE_IPP_
#define SPDNET_NET_EVENTSERVICE_IPP_

#include <spdnet/net/event_service.h>
#include <iostream>
#include <spdnet/net/socket_ops.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
    namespace net {
        const static unsigned int default_loop_timeout = 100;

        event_service::event_service() noexcept
                : random_(time(0)) {

        }

        event_service::~event_service() noexcept {
            stop();
        }

        std::shared_ptr<service_thread> event_service::get_service_thread() {
            auto rand_num = random_();
            return threads_[rand_num % threads_.size()];
        }

        void event_service::add_tcp_session(sock_t fd, bool is_server_side, const tcp_enter_callback &enter_callback,
                                            std::shared_ptr<service_thread> thread) {
            if (thread == nullptr)
                thread = get_service_thread();
            std::shared_ptr<tcp_session> new_session = tcp_session::create(fd, is_server_side, thread);
            thread->get_executor()->post([thread, new_session, enter_callback]() {
                /*
                if (loop->get_impl().on_socket_enter(*new_session->socket_data_)){
                    if (enter_callback)
                        enter_callback(new_session);
                }
                */
                thread->on_tcp_session_enter(new_session->sock_fd(), new_session, enter_callback);
            });
        }

        void event_service::run_thread(size_t thread_num) {
            if (thread_num <= 0)
                throw spdnet_exception(std::string("thread_num must > 0 "));

            run_thread_ = std::make_shared<bool>(true);
            for (size_t i = 0; i < thread_num; i++) {
                auto thread = std::make_shared<service_thread>(default_loop_timeout);
                thread->run(run_thread_);
                threads_.push_back(thread);
            }
        }

        void event_service::stop() {
            try {
                if (*run_thread_) {
                    *run_thread_ = false;
                    for (auto &thread : threads_) {
                        if (thread->get_thread()->joinable())
                            thread->get_thread()->join();
                    }

                    threads_.clear();
                }
            }
            catch (std::system_error &err) {
                (void) err;
            }


        }
    }
}

#endif //SPDNET_NET_EVENTSERVICE_IPP_