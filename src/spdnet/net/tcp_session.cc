#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/event_loop.h>
#
namespace spdnet {
    namespace net {
        TcpSession::TcpSession(std::shared_ptr<TcpSocket> socket, EventLoopPtr loop)
                : socket_(std::move(socket)),
                  loop_owner_(loop){

        }

        TcpSession::Ptr TcpSession::create(std::shared_ptr<TcpSocket> socket, EventLoopPtr loop) {
            return std::make_shared<TcpSession>(std::move(socket), loop);
        }

        void TcpSession::regWriteEvent() {
            struct epoll_event event{0, {nullptr}};
            event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
            event.data.ptr = (Channel *) (this);
            ::epoll_ctl(loop_owner_->epoll_fd(), EPOLL_CTL_MOD, socket_->sock_fd(), &event);
        }

        void TcpSession::unregWriteEvent() {
            struct epoll_event event{0, {nullptr}};
            event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
            event.data.ptr = (Channel *) (this);
            ::epoll_ctl(loop_owner_->epoll_fd(), EPOLL_CTL_MOD, socket_->sock_fd(), &event);
        }

        void TcpSession::send(const char *data, size_t len) {
            if (len <= 0)
                return;
            auto buffer = loop_owner_->getBufferBySize(len);
            assert(buffer);
            buffer->write(data, len);
            {
                std::lock_guard<SpinLock> lck(desciptor_data_.send_guard_);
                desciptor_data_.send_buffer_list_.push_back(buffer);
            }

            /*
            if (is_post_flush_) {
                return;
            }
            is_post_flush_ = true;
            auto this_ptr = shared_from_this();
            loop_owner_->runInEventLoop([this_ptr]() {
                if (this_ptr->is_can_write_) {
                    this_ptr->flushBuffer();
                    this_ptr->is_post_flush_ = false;
                }
            });
            */
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
                this_ptr->onClose();
            });
        }

        void TcpSession::postShutDown() {
            auto loop = loop_owner_;
            auto this_ptr = shared_from_this();
            loop_owner_->runInEventLoop([loop, this_ptr]() {
                loop->runAfterEventLoop([this_ptr]() {
                    this_ptr->execShutDownInLoop();
                });
            });
        }

        void TcpSession::execShutDownInLoop() {
            assert(socket_ != nullptr);
            assert(loop_owner_->isInLoopThread());
            if (socket_ != nullptr) {
                ::shutdown(socket_->sock_fd(), SHUT_WR);
            }
            is_can_write_ = false;
        }

        void TcpSession::setDisconnectCallback(TcpDisconnectCallback &&callback) {
            disconnect_callback_ = std::move(callback);
        }

        void TcpSession::setDataCallback(TcpDataCallback &&callback) {
            data_callback_ = std::move(callback);
        }

        void TcpSession::SocketDataDeleter::operator()(void* ptr) const
        {
            owner_->getImpl()->recycle_socket_data(ptr);
        }
    }
}