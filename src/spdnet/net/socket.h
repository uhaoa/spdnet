#ifndef SPDNET_NET_SOCKET_H_
#define SPDNET_NET_SOCKET_H_

#include <memory>
#include <spdnet/base/platform.h>
#include <spdnet/base/noncopyable.h>

namespace spdnet {
    namespace net {

        class TcpSocket : public spdnet::base::NonCopyable {
        public:
            explicit TcpSocket(sock fd);

            virtual ~TcpSocket();

            void setNoDelay() noexcept;

            bool setNonblock() noexcept;

            void setSendSize() noexcept;

            void setRecvSize() noexcept;

            sock sock_fd() const {
                return fd_;
            }

        private:
            sock fd_;
        };

        class ListenSocket : public spdnet::base::NonCopyable {
        public:
            explicit ListenSocket(sock fd);

            virtual ~ListenSocket();

            std::shared_ptr<TcpSocket> accept();

            bool setNonblock() noexcept;

            sock sock_fd() const {
                return fd_;
            }

        private:
            sock fd_;
            int idle_fd_;
        };
    }
}

#endif  // SPDNET_NET_SOCKET_H_