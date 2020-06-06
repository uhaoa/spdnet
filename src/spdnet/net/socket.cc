#include <spdnet/net/socket.h>
#include <spdnet/base/socket_api.h>
#include <spdnet/base/base.h>
#include <spdnet/net/exception.h>

namespace spdnet {
    namespace net {
        ListenSocket::ListenSocket(int fd)
                : fd_(fd),
                  idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {

        }

        ListenSocket::~ListenSocket() {
            base::closeSocket(fd_);
        }

        std::shared_ptr<TcpSocket> ListenSocket::accept() {
            int accept_fd = ::accept(fd_, nullptr, nullptr);
            if (accept_fd == -1) {
                if (errno == EMFILE) {
                    ::close(idle_fd_);
                    accept_fd = ::accept(fd_, nullptr, nullptr);
                    ::close(accept_fd);
                    idle_fd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
                }

                return nullptr;
            }
            return std::make_shared<TcpSocket>(accept_fd);
        }

        TcpSocket::TcpSocket(int fd)
                : fd_(fd) {

        }

        TcpSocket::~TcpSocket() {
            base::closeSocket(sock_fd());
        }

        void TcpSocket::setNoDelay() noexcept {
            base::socketNoDelay(sock_fd());
        }

        bool TcpSocket::setNonblock() noexcept {
            return base::socketNonBlock(sock_fd());
        }

        void TcpSocket::setSendSize() noexcept {

        }

        void TcpSocket::setRecvSize() noexcept {

        }

        bool ListenSocket::setNonblock() noexcept {
            return base::socketNonBlock(sock_fd());
        }


    }
}