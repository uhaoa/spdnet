#ifndef SPDNET_NET_ACCEPTOR_H_
#define SPDNET_NET_ACCEPTOR_H_

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/end_point.h>

namespace spdnet {
    namespace net {
        class EventService;

        class TcpAcceptor : public spdnet::base::NonCopyable {
        private:
#ifdef SPDNET_PLATFORM_LINUX
            class AcceptContext : public detail::Channel
            {
            public:
                friend class TcpAcceptor; 
                AcceptContext()
                {

                }
				void trySend() override {}
                void onClose() override {}
                void tryRecv() override 
                {

                }
                
            private:
                TcpAcceptor& acceptor_;
            };
#endif
        public:
            TcpAcceptor(EventService &service);

            void start(const EndPoint &addr, TcpSession::TcpEnterCallback &&cb);

        private:
            sock createListenSocket(const EndPoint &addr);
            void doAccept();
        private:
            EventService &service_;
            std::shared_ptr<ListenSocket> listen_socket_;
            TcpSession::TcpEnterCallback enter_callback_;
        };


    }
}


#endif  // SPDNET_NET_ACCEPTOR_H_