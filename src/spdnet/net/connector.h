#ifndef SPDNET_NET_CONNECTOR_H_
#define SPDNET_NET_CONNECTOR_H_
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/tcp_connection.h>

namespace spdnet
{
namespace net
{
	class TcpService;
	class ConnectSession;

    class AsyncConnector : public base::NonCopyable
    {
     public:
		friend ConnectSession; 
        using FailedCallback  = std::function<void()> ; 

		AsyncConnector(TcpService& service);
		~AsyncConnector();

        void asyncConnect(const std::string& ip , int port , TcpConnection::TcpEnterCallback&& success_cb , FailedCallback&& failed_cb);
	private:
        bool removeSession(int fd) ; 
    private:
		TcpService& service_; 
		std::mutex session_guard_;
		std::unordered_map<int, std::shared_ptr<ConnectSession>> connect_sessions_;
    };


}
}
#endif  // SPDNET_NET_CONNECTOR_H_