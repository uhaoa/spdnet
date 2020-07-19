#ifndef SPDNET_NET_CONNECT_CONTEXT_H_
#define SPDNET_NET_CONNECT_CONTEXT_H_

#include <mutex>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/end_point.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class connect_context : public channel {
            public:
                connect_context(sock_t fd, std::shared_ptr<service_thread> thread,
                               std::function<void()> &&success_cb, std::function<void()> &&failed_cb)
                        : fd_(fd), thread_(thread),
                          success_cb_(std::move(success_cb)),
                          failed_cb_(std::move(failed_cb)) {
                    reset();
                }

                void do_complete(size_t bytes_transferred, std::error_code ec) override {
                    if (!ec) {
                        assert(success_cb_ != nullptr);
                        success_cb_();
                        fd_ = invalid_socket;
                    } else {
                        // ...
                        assert(failed_cb_ != nullptr);
                        failed_cb_();
                        socket_ops::close_socket(fd_);
                    }
                }

                std::shared_ptr<service_thread> get_service_thread() {
                    return thread_;
                }

            private:
                sock_t fd_{invalid_socket};
                std::shared_ptr<service_thread> thread_;
                std::function<void()> success_cb_;
                std::function<void()> failed_cb_;
            };
        }
    }
}
#endif  // SPDNET_NET_CONNECT_CONTEXT_H_