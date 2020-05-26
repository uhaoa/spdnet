#ifndef SPDNET_NET_ACCEPTOR_H_
#define SPDNET_NET_ACCEPTOR_H_
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/tcp_connection.h>
namespace spdnet
{
namespace net
{
	class TcpService; 
    class TcpAcceptor : public base::NonCopyable 
    {
     public:
		 TcpAcceptor(TcpService& service);
		~TcpAcceptor();

        void start(const std::string& ip , int port , TcpConnection::TcpEnterCallback&& cb) ; 
    private:
        int createListenSocket() ; 
    private:
		TcpService& service_; 
	    std::string ip_ ; 
        int port_ ; 
        int epoll_fd_;
        std::shared_ptr<bool>  run_listen_ ; 
        std::shared_ptr<std::thread>      listen_thread_ ; 
    };


}
}


#endif  // SPDNET_NET_ACCEPTOR_H_