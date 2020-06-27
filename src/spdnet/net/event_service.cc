#include <spdnet/net/event_service.h>
#include <iostream>
#include <spdnet/net/socket_ops.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
    namespace net {
        const static unsigned int kDefaultLoopTimeout = 100;

        EventService::EventService() noexcept
                : random_(time(0)) {

        }

        EventService::~EventService() noexcept {
            stop();
        }

        std::shared_ptr<EventLoop> EventService::getEventLoop() {
            auto rand_num = random_();
            return loops_[rand_num % loops_.size()];
        }

        void EventService::addTcpSession(sock_t fd, const TcpSession::TcpEnterCallback &enter_callback) {
            std::shared_ptr<EventLoop> loop = getEventLoop();
            TcpSession::Ptr new_session = TcpSession::create(fd, loop);
            loop->post([loop, new_session, enter_callback]() {
                /*
                if (loop->getImpl().onSocketEnter(*new_session->socket_data_)){
                    if (enter_callback)
                        enter_callback(new_session);
                }
                */
                loop->onTcpSessionEnter(new_session, enter_callback); 
            });
        }

        void EventService::runThread(size_t thread_num) {
            if (thread_num <= 0)
                throw SpdnetException(std::string("thread_num must > 0 "));

            run_loop_ = std::make_shared<bool>(true);
            for (size_t i = 0; i < thread_num; i++) {
                auto loop = std::make_shared<EventLoop>(kDefaultLoopTimeout);
                loop->run(run_loop_);
                loops_.push_back(loop);
            }
        }

        void EventService::stop() {
            try {
                if (run_loop_) {
                    *run_loop_ = false;
                    for (auto &loop : loops_) {
                        if (loop->getLoopThread()->joinable())
                            loop->getLoopThread()->join();
                    }

                    loops_.clear();
                }
            }
            catch (std::system_error &err) {
                (void) err;
            }


        }
    }
}

