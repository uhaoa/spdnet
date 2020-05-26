#ifndef SPDNET_NET_SOCKET_H_
#define SPDNET_NET_SOCKET_H_
#include <memory>
#include <spdnet/base/base.h>
#include <spdnet/base/noncopyable.h>

namespace spdnet
{
namespace net
{

    class TcpSocket : public base::NonCopyable
    {
    public:
		explicit TcpSocket(int fd);
		virtual ~TcpSocket();

        void setNoDelay() noexcept; 
        bool setNonblock() noexcept ; 
        void setSendSize() noexcept ; 
        void setRecvSize() noexcept ; 

        int sock_fd()const {
            return fd_ ; 
        }
        
    private:
        int fd_ ; 
    };

    class ListenSocket : public base::NonCopyable 
    {
    public: 
		explicit ListenSocket(int fd);
		virtual ~ListenSocket();

		std::shared_ptr<TcpSocket> accept();
        bool setNonblock() noexcept;
        int sock_fd()const {
            return fd_ ; 
        }
    private: 
        int fd_ ; 
		int idle_fd_; 
    } ; 
}
}

#endif  // SPDNET_NET_SOCKET_H_