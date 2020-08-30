#ifndef SPDNET_NET_ACCEPTOR_H_
#define SPDNET_NET_ACCEPTOR_H_

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/end_point.h>
#include <spdnet/net/service_thread.h>

#ifdef SPDNET_PLATFORM_LINUX

#include <spdnet/net/detail/impl_linux/epoll_accept_channel.h>

#else
#include <spdnet/net/detail/impl_win/iocp_accept_channel.h>
#endif

#include <spdnet/net/ssl_env.h>

namespace spdnet {
    namespace net {
        class event_service;

        class tcp_acceptor : public spdnet::base::noncopyable {
        private:
            class accept_context;

            friend class accept_context;

        public:
            tcp_acceptor(event_service &service);

            ~tcp_acceptor();

            void start(const end_point &addr, tcp_enter_callback &&enter_cb);

            void stop();

            void setup_ssl_env(std::shared_ptr<ssl_environment> env)
            {
                ssl_env_ = env; 
            }
        private:
            sock_t create_listen_socket(const end_point &addr);

        private:
            event_service &service_;
            sock_t listen_fd_;
            end_point addr_;
            std::shared_ptr<service_thread> listen_thread_;
            std::shared_ptr<bool> run_listen_;
            std::shared_ptr<detail::accept_channel_impl> accept_channel_;
            std::shared_ptr<ssl_environment> ssl_env_; 
        };


    }
}

#include <spdnet/net/acceptor.ipp>

#endif  // SPDNET_NET_ACCEPTOR_H_