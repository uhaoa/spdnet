#ifndef SPDNET_NET_CHANNEL_H_
#define SPDNET_NET_CHANNEL_H_

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/base.h>
#include <spdnet/net/impl_linux/epoll_impl.h>
#include <spdnet/net/connector.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class EventLoop;
            class TcpSession;

            class Channel {
            public:
                virtual ~Channel() noexcept {}

            private:
                virtual void trySend() = 0;

                virtual void tryRecv() = 0;

                virtual void onClose() = 0;

                friend class EventLoop;
            };

            class TcpSessionChannel : public Channel {
            public:
                friend class EpollImpl ;
                TcpSessionChannel(TcpSession::Ptr session, EventLoop& loop_owner)
                        : session_(session), loop_owner_(loop_owner) {

                }

                void trySend() override {
                    loop_owner_.getImpl().flushBuffer(session_);
                    loop_owner_.getImpl().cancelWriteEvent(session_);
                }

                void tryRecv() override {
                    loop_owner_.getImpl().doRecv(session_);
                }

                void onClose() override {
                    loop_owner_.getImpl().closeSession(session);
                }

            private:
                TcpSession::Ptr session_;
                EventLoop& loop_owner_;
            } ;

            struct AcceptorChannel : public Channel {
                //
            };

            class ConnectorChannel : public Channel {
            public:
                ConnectorChannel(int clientfd , AsyncConnector& connector,EventLoop& loop_owner)
                    :client_fd_(clientfd) , connector_(connector) , loop_owner_(loop_owner)
                {

                }
                void trySend() override {
                    assert(loop_owner_.isInLoopThread());
                    int result = 0;
                    socklen_t result_len = sizeof(result);
                    if (SPDNET_PREDICT_FALSE(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &result, &result_len) == -1
                                             || result != 0
                                             || spdnet::base::checkSelfConnect(fd_))) {
                        if (failed_cb_ != nullptr)
                            failed_cb_();

                        connector_.onFailed(client_fd_);
                    }
                    else {
                        
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
                int client_fd_{-1};
                AsyncConnector& connector_;
                EventLoop& loop_owner_;
            };

            class WakeupChannel : public Channel {
            public:
                explicit WakeupChannel(int fd) noexcept;

                bool wakeup() {
                    int data = 1;
                    return ::write(fd_, &data, sizeof(data)) == 0;
                }

            private:
                void trySend() override {

                }

                void tryRecv() override {
                    char buf[1024]{0};
                    while (true) {
                        auto ret = ::read(fd_, buf, sizeof(buf));
                        if (ret == -1 || static_cast<size_t>(ret) < sizeof(buf))
                            break;
                    }
                }

                void onClose() override {

                }

            private:
                int fd_;
            };
        }
    }

}

#endif  // SPDNET_NET_CHANNEL_H_