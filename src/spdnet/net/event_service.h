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
#include <spdnet/net/event_loop.h>
#include <spdnet/net/env_init.h>

namespace spdnet {
    namespace net {
        class SPDNET_EXPORT EventService : public base::NonCopyable {
        public:

            EventService() noexcept;

            ~EventService() noexcept;

            void
            addTcpSession(sock_t fd, const TcpSession::TcpEnterCallback &enter_callback);

            void runThread(size_t thread_num);

            std::shared_ptr<EventLoop> getEventLoop();

        private:
            void stop();

        private:
            std::shared_ptr<bool> run_loop_;
            std::vector<std::shared_ptr<EventLoop>> loops_;
            std::mt19937 random_;
            EnvInit env_;
        };
    }
}


#endif // SPDNET_NET_EVENTSERVICE_H_
