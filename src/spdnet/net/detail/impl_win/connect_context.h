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
            class ConnectContext : public Channel {
            public:
                ConnectContext(sock_t fd, std::shared_ptr<ServiceThread> service_thread,
                               std::function<void()> &&success_cb, std::function<void()> &&failed_cb)
                        : fd_(fd), service_thread_(service_thread),
                          success_cb_(std::move(success_cb)),
                          failed_cb_(std::move(failed_cb)) {
                    reset();
                }

                void doComplete(size_t bytes_transferred, std::error_code ec) override {
                    if (!ec) {
                        assert(success_cb_ != nullptr);
                        success_cb_();
                        fd_ = invalid_socket;
                    } else {
                        // ...
                        assert(failed_cb_ != nullptr);
                        failed_cb_();
                        socket_ops::closeSocket(fd_);
                    }
                }

                std::shared_ptr<ServiceThread> getServiceThread() {
                    return service_thread_;
                }

            private:
                sock_t fd_{invalid_socket};
                std::shared_ptr<ServiceThread> service_thread_;
                std::function<void()> success_cb_;
                std::function<void()> failed_cb_;
            };
        }
    }
}
#endif  // SPDNET_NET_CONNECT_CONTEXT_H_