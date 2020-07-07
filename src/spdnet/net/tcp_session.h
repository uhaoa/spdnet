#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/socket_data.h>

namespace spdnet {
    namespace net {
        class ServiceThread;

        class TcpSession : public spdnet::base::NonCopyable, public std::enable_shared_from_this<TcpSession> {
        public:
            friend class ServiceThread;

            friend class EventService;

            using TcpDataCallback = std::function<size_t(const char *, size_t len)>;
            using TcpDisconnectCallback = std::function<void(std::shared_ptr<TcpSession>)>;
        public:
            inline TcpSession(sock_t fd, bool is_server_side, std::shared_ptr<ServiceThread> service_thread);

            inline ~TcpSession();

            inline void postShutDown();

            inline void setDisconnectCallback(TcpDisconnectCallback &&callback);

            inline void setDataCallback(TcpDataCallback &&callback) {
                socket_data_->setDataCallback(std::move(callback));
            }

            inline void setMaxRecvBufferSize(size_t len) {
                socket_data_->setMaxRecvBufferSize(len);
            }

            inline void setNodelay() {
                socket_data_->setNodelay();
            }

            inline void send(const char *data, size_t len);


            inline sock_t sock_fd() const {
                return socket_data_->sock_fd();
            }

        public:
            inline static std::shared_ptr<TcpSession>
            create(sock_t fd, bool is_server_side, std::shared_ptr<ServiceThread> service_thread);

        private:
            SocketData::Ptr socket_data_;
            std::shared_ptr<ServiceThread> service_thread_;
        };

    }
}

#include <spdnet/net/tcp_session.ipp>

#endif // SPDNET_NET_TCP_SESSION_H_