#include <spdnet/net/connector.h>
#include <unordered_map>
#include <chrono>
#include <set> 
#include <cassert>
#include <spdnet/base/socket_api.h>
#include <iostream>
#include <algorithm>
#include <spdnet/net/event_service.h>

namespace spdnet
{
namespace net
{

	class ConnectSession : public Channel
	{
	public:
		ConnectSession(int fd , AsyncConnector& connector , EventLoop::Ptr loop_owner , TcpSession::TcpEnterCallback&& enter_cb , AsyncConnector::FailedCallback&& failed_cb)
			:fd_(fd) , 
			 connector_(connector) ,
			 loop_owner_(loop_owner) , 
			 enter_cb_(std::move(enter_cb)) , 
			 failed_cb_(std::move(failed_cb))
		{

		}

		void trySend() override
		{
			assert(loop_owner_->isInLoopThread());

			auto& connector = connector_;
			auto fd = fd_;
			loop_owner_->runAfterEventLoop([&connector, &fd]() {
				connector.removeSession(fd);
			});


			int result = 0 ; 
			socklen_t result_len = sizeof(result);
			if (SPDNET_PREDICT_FALSE(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &result, &result_len) == -1
				|| result != 0
				|| spdnet::base::checkSelfConnect(fd_)))
			{
				if (failed_cb_ != nullptr)
					failed_cb_();

				spdnet::base::closeSocket(fd_); 
				return ; 
        	}
			
			cancelEvent();

			auto socket = std::make_shared<TcpSocket>(fd_);
			auto new_conn = TcpSession::create(std::move(socket) , loop_owner_);
			loop_owner_->onTcpSessionEnter(new_conn , enter_cb_);
		}

		void tryRecv() override
		{
			/*assert(false);*/
		}

		void onClose() override
		{
			cancelEvent();
			if (failed_cb_ != nullptr)
				failed_cb_();
			spdnet::base::closeSocket(fd_);
		}
	private:
		void cancelEvent() {
			struct epoll_event ev{0 , {nullptr}} ; 
			::epoll_ctl(loop_owner_->epoll_fd() , EPOLL_CTL_DEL , fd_ , &ev);
		}
	private:
		int fd_ {0};
		AsyncConnector& connector_; 
		EventLoop::Ptr loop_owner_;
		TcpSession::TcpEnterCallback enter_cb_;
		AsyncConnector::FailedCallback failed_cb_;
	};

	AsyncConnector::AsyncConnector(EventService& service)
		: service_(service)
	{

	}

    AsyncConnector::~AsyncConnector()
    {

    }

	bool AsyncConnector::removeSession(int fd) 
	{
		std::lock_guard<std::mutex> lck(session_guard_);
		return connect_sessions_.erase(fd) > 0;
	}

	void AsyncConnector::asyncConnect(const std::string& ip, int port, TcpSession::TcpEnterCallback&& enter_cb, FailedCallback&& failed_cb)
	{
		struct sockaddr_in server_addr = { 0 };
		spdnet::base::initSocketEnv();
		int client_fd = socket(PF_INET, SOCK_STREAM, 0);
		if (client_fd == -1)
			return;
		spdnet::base::socketNonBlock(client_fd);
		server_addr.sin_family = AF_INET;
		inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr.s_addr);
		server_addr.sin_port = htons(port);

		int ret = ::connect(client_fd, (struct sockaddr*) & server_addr, sizeof(struct sockaddr));
		if (ret == 0) {
			if (spdnet::base::checkSelfConnect(client_fd))
				goto FAILED;

			auto socket = std::make_shared<TcpSocket>(client_fd);
			service_.addTcpSession(socket, enter_cb);
		}
		else if (errno != EINPROGRESS) {
			goto FAILED;
		}
		else {
			auto loop = service_.getEventLoop();
			auto channel = std::make_shared<ConnectSession>(client_fd , *this , loop , std::move(enter_cb) , std::move(failed_cb));
			{
				std::lock_guard<std::mutex> lck(session_guard_);
				connect_sessions_[client_fd] = channel;
			}
			loop->linkChannel(client_fd, channel.get() , EPOLLET | EPOLLOUT | EPOLLRDHUP); 
			return; 
		}
		
	FAILED:
		if (client_fd != -1) {
			spdnet::base::closeSocket(client_fd);
			client_fd = -1;
		}
		if (failed_cb != nullptr) {
			failed_cb();
		}
    }


}
}