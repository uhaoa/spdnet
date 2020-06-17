#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/net/event_loop.h>

namespace spdnet {
    namespace net {
        using namespace base;

        class TcpSession : public base::NonCopyable, public std::enable_shared_from_this<TcpSession> {
        public:
            friend class EventLoop;
            using Ptr = std::shared_ptr<TcpSession>;
            using TcpDisconnectCallback = EventLoop:::IoImplType::TcpDisconnectCallback;
            using TcpDataCallback = EventLoop:::IoImplType::TcpDataCallback;
			using SocketImplDataType = EventLoop:::IoImplType::SocketImplData;
        
			using TcpEnterCallback = std::function<void(TcpSession::Ptr)>;
		public:
            TcpSession(std::shared_ptr<TcpSocket> socket, std::shared_ptr<EventLoop>);

            void postShutDown();

			void setDisconnectCallback(TcpDisconnectCallback&& callback) {
				/*
				auto cb = std::move(callback);
				auto this_ptr = shared_from_this();
				socket_data_->setImplCloseCallback([cb , this_ptr]() {
					if (cb)
						cb(this_ptr);
					this_ptr->lo	
				});
				*/
				socket_data_->setDisconnectCallback(callback); 
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

			/*
            int sock_fd() const {
                return socket_->sock_fd();
            }
			*/
        public:
            static Ptr create(std::shared_ptr<TcpSocket> socket, std::shared_ptr<EventLoop> loop);
        private:
            std::shared_ptr<EventLoop> loop_owner_;
            TcpDisconnectCallback disconnect_callback_;
            TcpDataCallback data_callback_;
            size_t max_recv_buffer_size_ = 64 * 1024;
            std::shared_ptr<SocketImplDataType> socket_data_;
        };

    }
}


#endif // SPDNET_NET_TCP_SESSION_H_