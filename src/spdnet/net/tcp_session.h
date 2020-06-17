#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#if defined SPDNET_PLATFORM_LINUX
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#endif

namespace spdnet {
    namespace net {
        using namespace base;
        class EventLoop;
        class TcpSession : public base::NonCopyable, public std::enable_shared_from_this<TcpSession> {
        public:
            friend class EventLoop;
            using Ptr = std::shared_ptr<TcpSession>;
            using TcpDataCallback = std::function<size_t(const char*, size_t len)>;
            using TcpDisconnectCallback = std::function<void(Ptr)>;
			using TcpEnterCallback = std::function<void(TcpSession::Ptr)>;
		public:
            TcpSession(std::shared_ptr<TcpSocket> socket, std::shared_ptr<EventLoop>);

            void postShutDown();

			void setDisconnectCallback(TcpDisconnectCallback&& callback) {
				auto cb = std::move(callback);
				auto this_ptr = shared_from_this();
				socket_data_->setDisconnectCallback([cb , this_ptr]() {
					if (cb)
						cb(this_ptr);
				});

				//socket_data_->setDisconnectCallback(callback);
			}

			void setDataCallback(TcpDataCallback&& callback) {
				socket_data_->setDataCallback(callback); 
			}

            void setMaxRecvBufferSize(size_t len) {
                max_recv_buffer_size_ = len;
            }

            void setNodelay() {
				socket_data_->setNodelay();
            }

            void send(const char *data, size_t len);


            int sock_fd() const {
                return socket_data_->sock_fd();
            }

        public:
            static Ptr create(std::shared_ptr<TcpSocket> socket, std::shared_ptr<EventLoop> loop);
        private:
            std::shared_ptr<EventLoop> loop_owner_;
            size_t max_recv_buffer_size_ = 64 * 1024;
#if defined SPDNET_PLATFORM_LINUX
            std::shared_ptr<detail::EpollImpl> socket_data_;
#endif

        };

    }
}


#endif // SPDNET_NET_TCP_SESSION_H_