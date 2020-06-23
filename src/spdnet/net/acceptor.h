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
			class AcceptContext;
			friend class AcceptContext; 
        public:
            TcpAcceptor(EventService &service);

            void start(const EndPoint &addr, TcpSession::TcpEnterCallback &&cb);

        private:
            sock createListenSocket(const EndPoint &addr);
			void onAcceptSuccess(std::shared_ptr<TcpSocket> new_socket);
        private:
            EventService &service_;
            std::shared_ptr<ListenSocket> listen_socket_;
            TcpSession::TcpEnterCallback enter_callback_;
			std::unique_ptr<AcceptContext> context_; 
        };


    }
}


#endif  // SPDNET_NET_ACCEPTOR_H_