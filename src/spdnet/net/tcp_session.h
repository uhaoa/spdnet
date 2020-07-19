#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_

#include <memory>
#include <deque>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/buffer.h>
#include <spdnet/base/spin_lock.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/socket_data.h>

namespace spdnet {
    namespace net {
        class service_thread;

        class tcp_session : public spdnet::base::noncopyable, public std::enable_shared_from_this<tcp_session> {
        public:
            friend class service_thread;

            friend class event_service;

            using tcp_data_callback = std::function<size_t(const char *, size_t len)>;
            using tcp_disconnect_callback = std::function<void(std::shared_ptr<tcp_session>)>;
        public:
            inline tcp_session(sock_t fd, bool is_server_side, std::shared_ptr<service_thread> service_thread);

            inline ~tcp_session();

            inline void post_shutdown();

            inline void set_disconnect_callback(tcp_disconnect_callback &&callback);

            inline void set_data_callback(tcp_data_callback &&callback) {
                socket_data_->set_data_callback(std::move(callback));
            }

            inline void set_max_recv_buffer_size(size_t len) {
                socket_data_->set_max_recv_buffer_size(len);
            }

            inline void set_no_delay() {
                socket_data_->set_no_delay();
            }

            inline void send(const char *data, size_t len);


            inline sock_t sock_fd() const {
                return socket_data_->sock_fd();
            }

        public:
            inline static std::shared_ptr<tcp_session>
            create(sock_t fd, bool is_server_side, std::shared_ptr<service_thread> service_thread);

        private:
            socket_data::ptr socket_data_;
            std::shared_ptr<service_thread> service_thread_;
        };

    }
}

#include <spdnet/net/tcp_session.ipp>

#endif // SPDNET_NET_TCP_SESSION_H_