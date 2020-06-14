#include <spdnet/net/detail/impl_linux/connector_impl.h>
#include <unordered_map>
#include <chrono>
#include <set>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <spdnet/base/socket_api.h>
#include <spdnet/net/event_service.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#include <spdnet/net/detail/impl_linux/channel.h>
#include <spdnet/net/connector.h>

namespace spdnet {
    namespace net {
        namespace detail {
        class ConnectContext : public Channel {
            public:
            ConnectContext(int fd, AsyncConnectorImpl &connector, std::shared_ptr<EventLoop> loop_owner,
                               TcpSession::TcpEnterCallback &&enter_cb,  std::function<void()> &&failed_cb)
                        : fd_(fd),
                          connector_(connector),
                          loop_owner_(loop_owner),
                          enter_cb_(std::move(enter_cb)),
                          failed_cb_(std::move(failed_cb)) {

            }
            void trySend() override {
                assert(loop_owner_->isInLoopThread());
                int result = 0;
                socklen_t result_len = sizeof(result);
                if (SPDNET_PREDICT_FALSE(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &result, &result_len) == -1
                                         || result != 0
                                         || spdnet::base::checkSelfConnect(fd_))) {
                    if (failed_cb_ != nullptr)
                        failed_cb_();
                    return ;
                }

                cancelEvent();

                auto socket = std::make_shared<TcpSocket>(fd_);
                auto new_conn = TcpSession::create(std::move(socket), loop_owner_);
                loop_owner_->onTcpSessionEnter(new_conn, enter_cb_);
            }

            void tryRecv() override {
                /*assert(false);*/
            }

            void onClose() override {
                cancelEvent();
                if (failed_cb_ != nullptr)
                    failed_cb_();
                spdnet::base::closeSocket(fd_);
            }
            private:
                void cancelEvent() {
                    struct epoll_event ev{0, {nullptr}};
                    ::epoll_ctl(loop_owner_->getImpl().epoll_fd(), EPOLL_CTL_DEL, fd_, &ev);
                }

            private:
                int fd_{0};
                AsyncConnectorImpl &connector_;
                std::shared_ptr<EventLoop> loop_owner_;
                TcpSession::TcpEnterCallback enter_cb_;
                std::function<void()> failed_cb_;
            };

            AsyncConnectorImpl::AsyncConnectorImpl(EventService &service)
                    : service_(service) {

            }

            bool AsyncConnectorImpl::removeContext(int fd) {
                std::lock_guard<std::mutex> lck(context_guard_);
                return connecting_context_.erase(fd) > 0;
            }

            void AsyncConnectorImpl::asyncConnect(const EndPoint &addr, TcpSession::TcpEnterCallback &&enter_cb,
                                                  std::function<void()> &&failed_cb) {
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
                    auto channel = std::make_shared<ConnectContext>(client_fd, *this, loop, std::move(enter_cb),
                                                                    std::move(failed_cb));
                    {
                        std::lock_guard<std::mutex> lck(context_guard_);
                        connecting_context_[client_fd] = channel;
                    }
                    loop->getImpl().linkChannel(client_fd, channel.get(), EPOLLET | EPOLLOUT | EPOLLRDHUP);
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
}