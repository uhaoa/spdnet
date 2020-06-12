#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/event_loop.h>
#
namespace spdnet {
    namespace net {
        TcpSession::TcpSession(std::shared_ptr<TcpSocket> socket, std::shared_ptr <EventLoop> loop)
                : socket_(std::move(socket)),
                  loop_owner_(loop){

        }

        TcpSession::Ptr TcpSession::create(std::shared_ptr<TcpSocket> socket, std::shared_ptr <EventLoop> loop) {
            return std::make_shared<TcpSession>(std::move(socket), loop);
        }

        void TcpSession::send(const char *data, size_t len) {
            if (len <= 0)
                return;
            auto buffer = loop_owner_->allocBufferBySize(len);
            assert(buffer);
            buffer->write(data, len);
			{
				std::lock_guard<SpinLock> lck(desciptor_data_.send_guard_);
				desciptor_data_.send_buffer_list_.push_back(buffer);
			}
        }


        void TcpSession::onClose() {
            assert(loop_owner_->isInLoopThread());
            auto loop = loop_owner_;
            auto callback = disconnect_callback_;
            Ptr session = shared_from_this();
            std::shared_ptr<TcpSocket> socket = std::move(socket_);
            loop_owner_->runAfterEventLoop([loop, callback, session, socket]() {
                if (callback)
                    callback(session);
                loop->removeTcpSession(socket->sock_fd());
                struct epoll_event ev{0, {nullptr}};
                ::epoll_ctl(loop->epoll_fd(), EPOLL_CTL_DEL, socket->sock_fd(), &ev);
            });
            disconnect_callback_ = nullptr;
            data_callback_ = nullptr;
            socket_.reset();
        }

        void TcpSession::postDisconnect() {
            auto loop = loop_owner_;
            auto this_ptr = shared_from_this();
            loop_owner_->runInEventLoop([loop, this_ptr]() {
				loop->getImpl().closeSession(this_ptr); 
            });
        }

        void TcpSession::postShutDown() {
            auto loop = loop_owner_;
            auto this_ptr = shared_from_this();
            loop_owner_->runInEventLoop([loop, this_ptr]() {
				loop->getImpl().shutdownSession(this_ptr); 
            });
        }

		/*
        void TcpSession::execShutDownInLoop() {
            assert(socket_ != nullptr);
            assert(loop_owner_->isInLoopThread());
            if (socket_ != nullptr) {
                ::shutdown(socket_->sock_fd(), SHUT_WR);
            }
            is_can_write_ = false;
        }
		*/

        void TcpSession::setDisconnectCallback(TcpDisconnectCallback &&callback) {
            disconnect_callback_ = std::move(callback);
        }

        void TcpSession::setDataCallback(TcpDataCallback &&callback) {
            data_callback_ = std::move(callback);
        }

        void TcpSession::SocketDataDeleter::operator()(void* ptr) const
        {
            owner_->getImpl().recycle_private_data(ptr);
        }
    }
}