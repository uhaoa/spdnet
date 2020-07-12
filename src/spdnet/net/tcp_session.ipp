#ifndef SPDNET_NET_TCP_SESSION_IPP_
#define SPDNET_NET_TCP_SESSION_IPP_

#include <spdnet/net/tcp_session.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/service_thread.h>

namespace spdnet {
    namespace net {
        TcpSession::TcpSession(sock_t fd, bool is_server_side, std::shared_ptr<ServiceThread> service_thread)
                : service_thread_(service_thread) {
            socket_data_ = std::make_shared<SocketData>(fd, is_server_side);
        }

        TcpSession::~TcpSession() {

        }

        std::shared_ptr<TcpSession>
        TcpSession::create(sock_t fd, bool is_server_side, std::shared_ptr<ServiceThread> service_thread) {
            return std::make_shared<TcpSession>(fd, is_server_side, service_thread);
        }

        void TcpSession::setDisconnectCallback(TcpDisconnectCallback &&callback) {
            auto cb = std::move(callback);
            auto this_ptr = shared_from_this();
            socket_data_->setDisconnectCallback([cb, this_ptr]() mutable {
                if (cb) {
                    cb(this_ptr);
                }
                this_ptr = nullptr;
                /*this_ptr->loop_owner_->removeTcpSession(this_ptr->sock_fd());*/
            });
        }

        void TcpSession::send(const char *data, size_t len) {
            if (len <= 0)
                return;
            /*
             *   这里采用裸指针进行传递 ，主要是避免shared_ptr频繁的原子操作引起的性能上的损失 。
             *   裸指针生命周期的安全性由SocketData和channel的相互引用保证 。
             *   当socket关闭时， channel不会立即被释放 ，而channel里引用了SocketData, 所以SocketData也不会被释放。
             *   channel的直正释放是被延迟到了epoll_wait和excutor之后 ， 所以channel和SocketData释放时，
             *   send函数投递的lamba肯定已经执行完了 , 因此lamba捕获的裸指针就不存在悬指针安全问题了。
             *   这种写法看起来很不舒服 ，但为了性能只能忍一忍了 ，哈哈
            **/
            service_thread_->getImpl()->send(socket_data_.get(), data, len);
        }

        void TcpSession::postShutDown() {
            auto this_ptr = shared_from_this();
            service_thread_->getExecutor()->post([this_ptr]() {
                this_ptr->service_thread_->getImpl()->shutdownSocket(this_ptr->socket_data_);
            });
        }

    }
}
#endif // SPDNET_NET_TCP_SESSION_IPP_