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

        EventService::EventService() noexcept
                : random_(time(0)) {

        }

        EventService::~EventService() noexcept {
            stop();
        }

        std::shared_ptr<ServiceThread> EventService::getServiceThread() {
            auto rand_num = random_();
            return threads_[rand_num % threads_.size()];
        }

        void EventService::addTcpSession(sock_t fd, bool is_server_side ,  const TcpEnterCallback &enter_callback) {
            std::shared_ptr<ServiceThread> service_thread = getServiceThread();
            std::shared_ptr<TcpSession> new_session = TcpSession::create(fd , is_server_side , service_thread);
            service_thread->getExecutor()->post([service_thread, new_session, enter_callback]() {
                /*
                if (loop->getImpl().onSocketEnter(*new_session->socket_data_)){
                    if (enter_callback)
                        enter_callback(new_session);
                }
                */
                service_thread->onTcpSessionEnter(new_session->sock_fd() , new_session, enter_callback);
            });
        }

        void EventService::runThread(size_t thread_num) {
            if (thread_num <= 0)
                throw SpdnetException(std::string("thread_num must > 0 "));

            run_thread_ = std::make_shared<bool>(true);
            for (size_t i = 0; i < thread_num; i++) {
                auto service_thread = std::make_shared<ServiceThread>(default_loop_timeout);
                service_thread->run(run_thread_);
                threads_.push_back(service_thread);
            }
        }

        void EventService::stop() {
            try {
                if (*run_thread_) {
                    *run_thread_ = false;
                    for (auto & service_thread : threads_) {
                        if (service_thread->getThread()->joinable())
                            service_thread->getThread()->join();
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