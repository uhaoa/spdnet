#ifndef SPDNET_NET_NETSERVICE_H_
#define SPDNET_NET_NETSERVICE_H_
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <random>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/base.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/event_loop.h>

namespace spdnet
{
namespace net
{
    class TcpService : public base::NonCopyable , public std::enable_shared_from_this<TcpService>
    {
	 private:
         struct ServiceParam
         {
               size_t             max_recv_buff_size() const { return max_recv_buff_size_ ; }
               bool               nodelay() const { return nodelay_ ; }
               const EventLoop::TcpEnterCallback& enter_callback() const { return enter_callback_ ;}

               size_t             max_recv_buff_size_ = 128;
               bool               nodelay_ = false ;
               EventLoop::TcpEnterCallback enter_callback_ ; 
         }; 
	public:
		using Ptr = std::shared_ptr<TcpService>;
		class ServiceParamBuilder
		{
		public:
			ServiceParamBuilder()
			{
				params_ = std::make_shared<ServiceParam>(); 
			}
			ServiceParamBuilder&  WithMaxRecvBuffSize(size_t size)
			{
				params_->max_recv_buff_size_ = size;
				return *this;
			}
			ServiceParamBuilder& WithNoDelay() {
				params_->nodelay_ = true; 
				return *this; 
			}
			ServiceParamBuilder& WithTcpEnterCallback(EventLoop::TcpEnterCallback&& enter_callback) {
				params_->enter_callback_ = std::move(enter_callback);
				return *this; 
			}
			std::shared_ptr<ServiceParam> GetParams() {
				return params_; 
			}
		private:
			std::shared_ptr<ServiceParam>     params_;
		};
     public:

		 TcpService() noexcept;
        ~TcpService() noexcept;

        static Ptr Create() ; 

		void StartServer(const std::string& ip, int port, std::shared_ptr<ServiceParam> params);
        void LaunchWorkerThread(size_t num);
        void Stop(); 

        EventLoop::Ptr GetRandomEventLoop(); 
        
      void AddTcpConnection(TcpSocket::Ptr , const EventLoop::TcpEnterCallback& enter_callback); 
     private:
        int Listen();
     private:
        std::string                       ip_ ; 
        int                               port_ ; 
        int                               listen_epoll_fd_;
        std::shared_ptr<bool>             run_listen_ ; 
        std::shared_ptr<bool>             run_loop_ ; 
        std::shared_ptr<std::thread>      listen_thread_ ; 
        std::mutex                        service_lock_ ; 
        std::shared_ptr<ServiceParam>     params_ ;     

        std::vector<EventLoop::Ptr>            loops ; 
        std::mt19937                           random_ ; 
    } ; 
}
}


#endif // SPDNET_NET_NETSERVICE_H_