#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/event_loop.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>

namespace spdnet {
    namespace net {
        TcpSession::TcpSession(std::shared_ptr<TcpSocket> socket, std::shared_ptr <EventLoop> loop)
                : socket_(std::move(socket)),
                  loop_owner_(loop){
            auto this_ptr = shared_from_this();
            auto data_ptr = std::make_shared<detail::SocketImplData>(this_ptr , *loop_owner_.get());
            socket_impl_data_ = std::move(data_ptr);
        }

        TcpSession::Ptr TcpSession::create(std::shared_ptr<TcpSocket> socket, std::shared_ptr <EventLoop> loop) {
            return std::make_shared<TcpSession>(std::move(socket), loop);
        }

        void TcpSession::send(const char *data, size_t len) {
            if (len <= 0)
                return;
            if (!socket_impl_data().is_can_write_)
                return ;
            auto buffer = loop_owner_->allocBufferBySize(len);
            assert(buffer);
            buffer->write(data, len);
			{
				std::lock_guard<SpinLock> lck(socket_impl_data().send_guard_);
                socket_impl_data().send_buffer_list_.push_back(buffer);
			}
        }


        void TcpSession::onClose() {
            assert(loop_owner_->isInLoopThread());
            auto session = shared_from_this();
            if (disconnect_callback_)
                disconnect_callback_(session);
            disconnect_callback_ = nullptr;
            data_callback_ = nullptr;
            socket_.reset();
        }
/*
        void TcpSession::postDisconnect() {
            auto loop = loop_owner_;
            auto this_ptr = shared_from_this();
            loop_owner_->runInEventLoop([loop, this_ptr]() {
				loop->getImpl().closeSession(this_ptr); 
            });
        }
*/
        void TcpSession::postShutDown() {
            auto loop = loop_owner_;
            auto this_ptr = shared_from_this();
            loop_owner_->runInEventLoop([loop, this_ptr]() {
				loop->getImpl().shutdownSession(this_ptr); 
            });
        }

        void TcpSession::setDisconnectCallback(TcpDisconnectCallback &&callback) {
            disconnect_callback_ = std::move(callback);
        }

        void TcpSession::setDataCallback(TcpDataCallback &&callback) {
            data_callback_ = std::move(callback);
        }
        
    }
}