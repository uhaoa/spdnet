#include <spdnet/net/socket.h>
#include <spdnet/base/socket_api.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/exception.h>
#include <iostream>

namespace spdnet {
    namespace net {
        ListenSocket::ListenSocket(sock_t fd)
                : fd_(fd),
#ifdef SPDNET_PLATFORM_LINUX
                  idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) 
#endif 
        {

        }

        ListenSocket::~ListenSocket() {
			socket_ops::closeSocket(fd_);
#ifdef SPDNET_PLATFORM_LINUX
			socket_ops::closeSocket(idle_fd_);
#endif 
		}

        std::shared_ptr<TcpSocket> ListenSocket::accept() {
			sock_t accept_fd = ::accept(fd_, nullptr, nullptr);
            if (accept_fd == -1) {
#ifdef SPDNET_PLATFORM_LINUX
                if (errno == EMFILE) {
                    ::close(idle_fd_);
                    accept_fd = ::accept(fd_, nullptr, nullptr);
                    ::close(accept_fd);
                    idle_fd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
                }
#endif
                std::cerr << "accept error . errno:" << current_errno() << std::endl; 
                // or throw exception ? 
                return nullptr;
            }
            return std::make_shared<TcpSocket>(accept_fd);
        }

		bool ListenSocket::setNonblock() noexcept {
			return socket_ops::socketNonBlock(sock_fd());
		}


        TcpSocket::TcpSocket(sock_t fd)
                : fd_(fd) {

        }

        TcpSocket::~TcpSocket() {
			socket_ops::closeSocket(sock_fd());
        }

        void TcpSocket::setNoDelay() noexcept {
			socket_ops::socketNoDelay(sock_fd());
        }

        bool TcpSocket::setNonblock() noexcept {
            return socket_ops::socketNonBlock(sock_fd());
        }

        void TcpSocket::setSendSize() noexcept {

        }

        void TcpSocket::setRecvSize() noexcept {

        }

    }
}