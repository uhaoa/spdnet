#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/event_loop.h>

#if defined SPDNET_PLATFORM_LINUX
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#endif

namespace spdnet {
    namespace net {
        TcpSession::TcpSession(std::shared_ptr<TcpSocket> socket, std::shared_ptr<EventLoop> loop)
                :loop_owner_(loop) {
			socket_data_ = std::make_shared<SocketImplDataType>(loop->getImpl(), std::move(socket));
        }

        TcpSession::Ptr TcpSession::create(std::shared_ptr<TcpSocket> socket, std::shared_ptr<EventLoop> loop) {
            return std::make_shared<TcpSession>(std::move(socket), loop);
        }

        void TcpSession::send(const char *data, size_t len) {
            if (len <= 0)
                return;
			
            /*
			if (!socket_data_->is_can_write_)
                return;
			
            auto buffer = loop_owner_->allocBufferBySize(len);
            assert(buffer);
            buffer->write(data, len);
            {
                std::lock_guard<SpinLock> lck(socket_impl_data().send_guard_);
				socket_data_->send_buffer_list_.push_back(buffer);
            }
			*/
            loop_owner_->getImpl().send(*socket_data_ , data , len);
        }

        void TcpSession::postShutDown() {
            auto loop = loop_owner_;
            auto this_ptr = shared_from_this();
            loop_owner_->post([loop, this_ptr]() {
                loop->getImpl().shutdownSocket(*this_ptr->socket_data_);
            });
        }

    }
}