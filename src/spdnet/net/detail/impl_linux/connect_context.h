#ifndef SPDNET_NET_CONNECT_CONTEXT_H_
#define SPDNET_NET_CONNECT_CONTEXT_H_

#include <mutex>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_linux/epoll_channel.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class connect_context : public detail::channel {
            public:
                connect_context(sock_t fd, std::shared_ptr<service_thread> thread,
                                std::function<void()> &&success_cb,
                                std::function<void()> &&failed_cb)
                        : fd_(fd), service_thread_(thread),
                          success_cb_(std::move(success_cb)),
                          failed_cb_(std::move(failed_cb)) {

                }

                void on_send() override {
                    cancel_event();
                    int result = 0;
                    socklen_t result_len = sizeof(result);
                    if (SPDNET_PREDICT_FALSE(
                            getsockopt(fd_, SOL_SOCKET, SO_ERROR, &result, &result_len) == SPDNET_SOCKET_ERROR
                            || result != 0
                            || socket_ops::check_self_connect(fd_))) {
                        assert(failed_cb_ != nullptr);
                        failed_cb_();
                        socket_ops::close_socket(fd_);
                        fd_ = invalid_socket;
                        return;
                    } else {
                        /*
                        auto new_conn = tcp_session::create(fd_, loop_owner_);
                        loop_owner_->on_tcp_session_enter(new_conn, enter_cb_);
                        */
                        assert(success_cb_ != nullptr);
                        success_cb_();
                    }
                }

                void on_recv() override {
                    /*assert(false);*/
                }

                void on_close() override {
                    assert(failed_cb_ != nullptr);
                    failed_cb_();
                    cancel_event();
                    socket_ops::close_socket(fd_);
                    fd_ = invalid_socket;
                }

                void cancel_event() {
                    struct epoll_event ev{0, {nullptr}};
                    ::epoll_ctl(service_thread_->get_impl()->epoll_fd(), EPOLL_CTL_DEL, fd_, &ev);
                }

                std::shared_ptr<service_thread> get_service_thread() {
                    return service_thread_;
                }

            private:
                sock_t fd_{invalid_socket};
                std::shared_ptr<service_thread> service_thread_;
                std::function<void()> success_cb_;
                std::function<void()> failed_cb_;
            };
        }
    }
}
#endif  // SPDNET_NET_CONNECT_CONTEXT_H_