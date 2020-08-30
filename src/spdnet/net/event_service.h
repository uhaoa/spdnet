#ifndef SPDNET_NET_EVENTSERVICE_H_
#define SPDNET_NET_EVENTSERVICE_H_

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <random>
#include <iostream>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/service_thread.h>
#include <spdnet/net/env_init.h>
#include <spdnet/net/ssl_env.h>
namespace spdnet {
    namespace net {
        class event_service : public base::noncopyable {
        public:

            event_service() noexcept;

            ~event_service() noexcept;

            void
            add_tcp_session(sock_t fd, bool is_server_side, const tcp_enter_callback &enter_callback,
                            std::shared_ptr<service_thread> thread = nullptr , std::shared_ptr<ssl_environment> ssl_env = nullptr);

            void run_thread(size_t thread_num);

            std::shared_ptr<service_thread> get_service_thread();

        private:
            void stop();

        private:
            std::shared_ptr<bool> run_thread_;
            std::vector<std::shared_ptr<service_thread>> threads_;
            std::mt19937 random_;
            env_init env_;
        };
    }
}

#include <spdnet/net/event_service.ipp>

#endif // SPDNET_NET_EVENTSERVICE_H_
