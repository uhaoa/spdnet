#ifndef SPDNET_NET_ACCEPTOR_H_
#define SPDNET_NET_ACCEPTOR_H_

#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/end_point.h>
#include <spdnet/net/service_thread.h>

namespace spdnet {
    namespace net {
        class EventService;

        class TcpAcceptor : public spdnet::base::NonCopyable {
		private:
			class AcceptContext;
			friend class AcceptContext; 
        public:
            TcpAcceptor(EventService &service);

            void start(const EndPoint &addr, TcpEnterCallback &&cb);

        private:
            sock_t createListenSocket(const EndPoint &addr);
			void onAcceptSuccess(sock_t new_socket);
        private:
            EventService &service_;
            //std::shared_ptr<ListenSocket> listen_socket_;
            sock_t listen_fd_;
			EndPoint addr_; 
            TcpEnterCallback enter_callback_;
			std::shared_ptr<AcceptContext> context_; 
        };


    }
}

#include <spdnet/net/acceptor.ipp>

#endif  // SPDNET_NET_ACCEPTOR_H_