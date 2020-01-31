#include <spdnet/net/socket.h>
#include <spdnet/base/socket_api.h>
#include <spdnet/base/base.h>
#include <spdnet/net/exception.h>

namespace spdnet
{
namespace net
{
    ListenSocket::ListenSocket(int fd):
            fd_(fd)
    {

    }

    ListenSocket::~ListenSocket()
    {
        base::CloseSocket(fd_); 
    }

    ListenSocket::Ptr ListenSocket::Create(int fd)
    {
        return Ptr(new ListenSocket(fd)); 
    }

    TcpSocket::Ptr ListenSocket::Accept() noexcept
    {   
        int acceptFd  = ::accept(fd_ , nullptr , nullptr); 
        if(acceptFd == -1)
        {
            return nullptr ; 
        }

        return TcpSocket::Create(acceptFd) ; 
    }

    TcpSocket::TcpSocket(int fd):
        fd_(fd)
    {

    }
    
    TcpSocket::~TcpSocket()
    {
        base::CloseSocket(sock_fd()) ; 
    }

    TcpSocket::Ptr TcpSocket::Create(int fd) 
    {
        return Ptr(new TcpSocket(fd)); 
    }


    void TcpSocket::SetNoDelay()noexcept
    {
        base::SetSocketNoDelay(sock_fd());
    }

    bool TcpSocket::SetNonblock() noexcept
    {
        return base::SetSocketNonBlock(sock_fd());
    }

    void TcpSocket::SetSendSize()noexcept
    {

    }

    void TcpSocket::SetRecvSize()noexcept
    {

    }

    bool ListenSocket::SetNonblock() 
    {
        return base::SetSocketNonBlock(sock_fd());
    }




}
}