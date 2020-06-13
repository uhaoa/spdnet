#include <spdnet/net/connector.h>
#include <unordered_map>
#include <chrono>
#include <set>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <spdnet/base/socket_api.h>
#include <spdnet/net/event_service.h>
#include <spdnet/net/impl_linux/epoll_impl.h>

namespace spdnet {
    namespace net {

        class ConnectContext {
        public:
            ConnectContext(int fd, AsyncConnector &connector, std::shared_ptr<EventLoop> loop_owner,
                           TcpSession::TcpEnterCallback &&enter_cb, AsyncConnector::FailedCallback &&failed_cb)
                    : fd_(fd),
                      connector_(connector),
                      loop_owner_(loop_owner),
                      enter_cb_(std::move(enter_cb)),
                      failed_cb_(std::move(failed_cb)) {

            }

        private:
            void cancelEvent() {
                struct epoll_event ev{0, {nullptr}};
                ::epoll_ctl(loop_owner_->epoll_fd(), EPOLL_CTL_DEL, fd_, &ev);
            }

        private:
            int fd_{0};
            AsyncConnector &connector_;
            std::shared_ptr<EventLoop> loop_owner_;
            TcpSession::TcpEnterCallback enter_cb_;
            AsyncConnector::FailedCallback failed_cb_;
        };

        AsyncConnector::AsyncConnector(EventService &service)
                : service_(service) {

        }

        AsyncConnector::~AsyncConnector() {

        }

        bool AsyncConnector::removeSession(int fd) {
            std::lock_guard<std::mutex> lck(session_guard_);
            return connecting_sessions_.erase(fd) > 0;
        }

        void AsyncConnector::asyncConnect(const EndPoint &addr, TcpSession::TcpEnterCallback &&enter_cb,
                                          FailedCallback &&failed_cb) {
            int client_fd = ::socket(addr.family(), SOCK_STREAM, 0);
            if (client_fd == -1)
                return;
            spdnet::base::socketNonBlock(client_fd);
            int ret = ::connect(client_fd, addr.socket_addr(), addr.socket_addr_len());
            if (ret == 0) {
                if (spdnet::base::checkSelfConnect(client_fd))
                    goto FAILED;

                auto socket = std::make_shared<TcpSocket>(client_fd);
                service_.addTcpSession(socket, enter_cb);
            } else if (errno != EINPROGRESS) {
                goto FAILED;
            } else {
                auto loop = service_.getEventLoop();
                auto channel = std::make_shared<ConnectSession>(client_fd, *this, loop, std::move(enter_cb),
                                                                std::move(failed_cb));
                {
                    std::lock_guard<std::mutex> lck(session_guard_);
                    connecting_sessions_[client_fd] = channel;
                }
                loop->getImpl().asyncConnect()
                //loop->linkChannel(client_fd, channel.get(), EPOLLET | EPOLLOUT | EPOLLRDHUP);
                return;
            }

            FAILED:
            if (client_fd != -1) {
                spdnet::base::closeSocket(client_fd);
                client_fd = -1;
            }
            if (failed_cb != nullptr) {
                failed_cb();
            }
        }


    }
}