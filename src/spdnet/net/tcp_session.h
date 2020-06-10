#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/channel.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>

namespace spdnet {
    namespace net {
        using namespace base;
        using namespace impl;
        class EventLoop;
        class DescriptorData;

        class TcpSession : public Channel, public base::NonCopyable, public std::enable_shared_from_this<TcpSession> {
        public:
            friend class EventLoop;
            friend class EpollImpl;

            using Ptr = std::shared_ptr<TcpSession>;
            using EventLoopPtr = std::shared_ptr<EventLoop>;
            using TcpDisconnectCallback = std::function<void(Ptr)>;
            using TcpDataCallback = std::function<size_t(const char *, size_t len)>;
            using TcpEnterCallback = std::function<void(TcpSession::Ptr)>;

            TcpSession(std::shared_ptr<TcpSocket> socket, EventLoopPtr);

            void postShutDown();

            void postDisconnect();

            void setDisconnectCallback(TcpDisconnectCallback &&callback);

            void setDataCallback(TcpDataCallback &&callback);

            void setMaxRecvBufferSize(size_t len) {
                max_recv_buffer_size_ = len;
            }

            void setNodelay() {
                socket_->setNoDelay();
            }

            void regWriteEvent();

            void unregWriteEvent();

            void send(const char *data, size_t len);

        public:
            static Ptr create(std::shared_ptr<TcpSocket> socket, EventLoopPtr loop);

        private:
            void flushBuffer();
			
            void trySend() override;

            void tryRecv() override;

            void onClose() override;
			
			void execShutDownInLoop();
			
            int getSocketFd() const {
                return socket_->sock_fd();
            }

            DescriptorData& desciptor_data() {
                return *desciptor_data_.get() ;
            }

        private:
            std::shared_ptr<TcpSocket> socket_;
            EventLoopPtr loop_owner_;
            TcpDisconnectCallback disconnect_callback_;
            TcpDataCallback data_callback_;
            size_t max_recv_buffer_size_ = 64 * 1024;
            Buffer recv_buffer_;
            std::deque<Buffer *> send_buffer_list_;
            std::deque<Buffer *> pending_buffer_list_;
            SpinLock send_guard_;
            volatile bool has_closed = false;

            /////////////////////////////////
            std::unique_ptr<DescriptorData> desciptor_data_ ;
            /////////////////////////////////

            volatile bool is_post_flush_;
            volatile bool is_can_write_;

        };

    }
}


#endif // SPDNET_NET_TCP_SESSION_H_