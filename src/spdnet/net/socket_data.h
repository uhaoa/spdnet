#ifndef SPDNET_NET_SOCKET_DATA_H_
#define SPDNET_NET_SOCKET_DATA_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/platform.h>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/net/socket_ops.h>
#include <spdnet/base/buffer_pool.h>

namespace spdnet {
    namespace net {
        namespace detail {
#if defined(SPDNET_PLATFORM_WINDOWS)
            class IocpRecvChannel;
            class IocpSendChannel;
#else

            class EPollSocketChannel;

#endif
        }
        class SocketData : public spdnet::base::NonCopyable {
        public:
            using Ptr = std::shared_ptr<SocketData>;
            using TcpDataCallback = std::function<size_t(const char *, size_t len)>;
            using TcpDisconnectCallback = std::function<void()>;

            SocketData(sock_t fd, bool is_server_side)
                    : fd_(fd), is_server_side_(is_server_side) {

            }

            virtual ~SocketData() {
                for (auto buffer : send_buffer_list_) {
                    base::BufferPool::instance().recycleBuffer(buffer);
                }
                for (auto buffer : pending_buffer_list_) {
                    base::BufferPool::instance().recycleBuffer(buffer);
                }
                send_buffer_list_.clear();
                pending_buffer_list_.clear();
            }

            void setDisconnectCallback(TcpDisconnectCallback &&callback) {
                disconnect_callback_ = callback;
            }

            void setDataCallback(TcpDataCallback &&callback) {
                data_callback_ = callback;
            }

            void setNodelay() {
                socket_ops::socketNoDelay(fd_);
            }

            sock_t sock_fd() const {
                return fd_;
            }

            void setMaxRecvBufferSize(size_t max_size) {
                max_recv_buffer_size_ = max_size;
            }

            bool isServerSide() { return is_server_side_; }

            void close() {
#if defined(SPDNET_PLATFORM_WINDOWS)
			    recv_channel_ = nullptr;
			    send_channel_ = nullptr;
#else
				channel_ = nullptr;
#endif
				if (disconnect_callback_)
					disconnect_callback_();

                disconnect_callback_ = nullptr; 
                data_callback_ = nullptr; 

				has_closed_ = true;
				is_can_write_ = false;

                socket_ops::closeSocket(fd_);
            }

        public:
            sock_t fd_;
            bool is_server_side_{false};
            TcpDisconnectCallback disconnect_callback_;
            TcpDataCallback data_callback_;
            spdnet::base::Buffer recv_buffer_;
            size_t max_recv_buffer_size_ = 64 * 1024;
            std::deque<spdnet::base::Buffer *> send_buffer_list_;
            std::deque<spdnet::base::Buffer *> pending_buffer_list_;
            spdnet::base::SpinLock send_guard_;
            volatile bool has_closed_{false};
            volatile bool is_post_flush_{false};
            volatile bool is_can_write_{true};

#if defined(SPDNET_PLATFORM_WINDOWS)
            std::shared_ptr<detail::IocpRecvChannel> recv_channel_;
            std::shared_ptr<detail::IocpSendChannel> send_channel_;
#else
            std::shared_ptr<detail::EPollSocketChannel> channel_;
#endif
        };
    }

}
#endif  // SPDNET_NET_SOCKET_DATA_H_