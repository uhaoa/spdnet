#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>

namespace spdnet {
    namespace net {

        namespace detail {
            struct SocketImplData;
            class EpollImpl ;
        }

        using namespace base;
        class EventLoop;
        class TcpSession :public base::NonCopyable, public std::enable_shared_from_this<TcpSession> {
        public:
            friend class EventLoop;
            friend class detail::EpollImpl;

            using Ptr = std::shared_ptr<TcpSession>;
            using TcpDisconnectCallback = std::function<void(Ptr)>;
            using TcpDataCallback = std::function<size_t(const char *, size_t len)>;
            using TcpEnterCallback = std::function<void(TcpSession::Ptr)>;

            TcpSession(std::shared_ptr<TcpSocket> socket, std::shared_ptr <EventLoop>);

            void postShutDown();

            void setDisconnectCallback(TcpDisconnectCallback &&callback);

            void setDataCallback(TcpDataCallback &&callback);

            void setMaxRecvBufferSize(size_t len) {
                max_recv_buffer_size_ = len;
            }

            void setNodelay() {
                socket_->setNoDelay();
            }

            void send(const char *data, size_t len);

			int sock_fd() const {
				return socket_->sock_fd();
			}
        public:
            static Ptr create(std::shared_ptr<TcpSocket> socket, std::shared_ptr<EventLoop> loop);

        private:
            void onClose();
            detail::SocketImplData& socket_impl_data() { return *socket_impl_data_.get();}
        private:
            std::shared_ptr<TcpSocket> socket_;
			std::shared_ptr <EventLoop> loop_owner_;
            TcpDisconnectCallback disconnect_callback_;
            TcpDataCallback data_callback_;
            size_t max_recv_buffer_size_ = 64 * 1024;
            std::shared_ptr<detail::SocketImplData> socket_impl_data_;
        };

    }
}


#endif // SPDNET_NET_TCP_SESSION_H_