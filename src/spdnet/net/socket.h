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
    private :
        class TcpSocketDeleter{
        public:
            void operator()(TcpSocket* ptr)const
            {
                delete ptr ; 
            }
        } ; 
    public :
        using Ptr = std::unique_ptr<TcpSocket , TcpSocketDeleter> ; 
    public:
        static Ptr Create(int fd) ; 
    public:
        void SetNoDelay() noexcept; 
        bool SetNonblock() noexcept ; 
        void SetSendSize() noexcept ; 
        void SetRecvSize() noexcept ; 

        int sock_fd()const {
            return fd_ ; 
        }
        
    protected :
        explicit TcpSocket(int fd);
        virtual ~TcpSocket();
    private:
        int fd_ ; 
    };

    class ListenSocket : public base::NonCopyable 
    {
    private:
        class ListenSocketDeleter{
        public:
            void operator()(ListenSocket* ptr)const
            {
                delete ptr ; 
            }
        };
    public:
        using Ptr = std::unique_ptr<ListenSocket , ListenSocketDeleter> ; 
        static Ptr Create(int fd); 
    public: 
        TcpSocket::Ptr Accept() noexcept; 
        bool SetNonblock() ; 
        int sock_fd()const {
            return fd_ ; 
        }
        
    protected:
        explicit ListenSocket(int fd);
        virtual ~ListenSocket() ; 
    private: 
        int fd_ ; 
    } ; 
}
}

#endif  // SPDNET_NET_SOCKET_H_