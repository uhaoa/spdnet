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

namespace spdnet {
    namespace net {
        class EventService : public base::NonCopyable {
        public:

            EventService() noexcept;

            ~EventService() noexcept;

            void
            addTcpSession(sock_t fd, bool is_server_side , const TcpEnterCallback &enter_callback);

            void runThread(size_t thread_num);

            std::shared_ptr<ServiceThread> getServiceThread();

        private:
            void stop();
        private:
            std::shared_ptr<bool> run_thread_;
            std::vector<std::shared_ptr<ServiceThread>> threads_;
            std::mt19937 random_;
            EnvInit env_;
        };
    }
}

#include <spdnet/net/event_service.ipp>
#endif // SPDNET_NET_EVENTSERVICE_H_
