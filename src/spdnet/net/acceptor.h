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

namespace spdnet {
    namespace net {
        class EventService;

        class TcpAcceptor : public spdnet::base::NonCopyable {
        private:
            class AcceptContext;

            friend class AcceptContext;

        public:
            TcpAcceptor(EventService &service);

            ~TcpAcceptor();

            void start(const EndPoint &addr, TcpEnterCallback &&enter_cb);

            void stop();

        private:
            sock_t createListenSocket(const EndPoint &addr);

        private:
            EventService &service_;
            sock_t listen_fd_;
            EndPoint addr_;
            std::shared_ptr<ServiceThread> listen_thread_;
            std::shared_ptr<bool> run_listen_;
            std::shared_ptr<detail::AcceptChannelImpl> accept_channel_;
        };


    }
}

#include <spdnet/net/acceptor.ipp>

#endif  // SPDNET_NET_ACCEPTOR_H_