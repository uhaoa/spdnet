#include <spdnet/net/connector.h>
#include <unordered_map>
#include <chrono>
#include <set> 
#include <cassert>
#include <poll.h>
#include <spdnet/base/socket_api.h>
#include <iostream>
#include <algorithm>

namespace spdnet
{
namespace net
{
    class ConnectAddress final 
    {
     public:
        using Ptr = std::shared_ptr<ConnectAddress> ;
		ConnectAddress(std::string&& ip , int port ,
                         AsyncConnector::SuccessCallback&& success_callback , AsyncConnector::FailedCallback&& failed_callback )
            :ip_(std::move(ip)) , 
             port_(port) , 
             success_callback_(std::move(success_callback)) , 
             failed_callback_(std::move(failed_callback))
        {

        }

        const std::string& ip()const{ return ip_; }
        int port()const{ return port_ ; }
        const AsyncConnector::SuccessCallback& success_callback()const{ return success_callback_ ; }
        const AsyncConnector::FailedCallback& failed_callback()const{ return failed_callback_ ; }
     private:
        std::string ip_ ; 
        int port_ ; 
        AsyncConnector::SuccessCallback success_callback_ ; 
        AsyncConnector::FailedCallback  failed_callback_ ; 
    };

    class ConnectorContext final : public base::NonCopyable
    {
	public:
		enum CheckType
		{
			CheckRead = 0x01,
			CheckWrite = 0x02,
			CheckError = 0x04,
		};
#define CHECK_READ_FLAG (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI)
#define CHECK_WRITE_FLAG (POLLOUT | POLLWRNORM | POLLWRBAND)
#define CHECK_ERROR_FLAG (POLLERR | POLLHUP)

    public:
        using Ptr = std::shared_ptr<ConnectorContext> ;

		ConnectorContext() noexcept ;
        void ProcessConnectTask(const ConnectAddress::Ptr& addr);
        void RunOnceCheckConnect(int millsecond); 
        bool CheckConnectSuccess(int fd , bool check_write) ;
        bool CheckSelfConnect(int fd);
        //staitc Ptr Create() ; 
	public:
		struct pollfd* FindPollfd(int fd);
		void JoinPoll(int fd, CheckType check_type);
		bool CheckEvent(const struct pollfd* pf, CheckType type); 
		int StartPoll(int timeout);
    private:
        
    private:
        class ConnectingTask
        {
         public:
			 ConnectingTask()
            {
                timeout_ = std::chrono::nanoseconds::zero();
            }
            std::chrono::steady_clock::time_point   start_connect_time_;
            std::chrono::nanoseconds                timeout_;

            std::shared_ptr<ConnectAddress>       server_addr_ ;
        };

        std::unordered_map<int , ConnectingTask> connecting_tasks_;
        std::vector<struct pollfd>				pollfds_;

    };


	ConnectorContext::ConnectorContext() noexcept
    {

    }  
/*
    ConnectorContext::Ptr ConnectorContext::Create() 
    {   
        return std::make_shared<ConnectorContext>();
    }
 */

	struct pollfd* ConnectorContext::FindPollfd(int fd)
	{
		auto result = std::find_if(pollfds_.begin(), pollfds_.end(), [fd](struct pollfd& self)->bool {
			return self.fd == fd;
		});
		if (result != pollfds_.end())
			return &*result;
		else
			return nullptr;

	}

	void ConnectorContext::JoinPoll(int fd, CheckType check_type)
	{
		auto pf = FindPollfd(fd);
		if (nullptr == pf) {
			struct pollfd tmp;
			tmp.fd = fd;
			tmp.events = 0;
			pollfds_.emplace_back(tmp);
			pf = &pollfds_[pollfds_.size() - 1];
		}
		assert(nullptr != pf);
		if (check_type & CheckRead)
			pf->events |= CHECK_READ_FLAG;
		if (check_type & CheckWrite)
			pf->events |= CHECK_WRITE_FLAG;
		if (check_type & CheckError)
			pf->events |= CHECK_ERROR_FLAG;
	}

	bool ConnectorContext::CheckEvent(const struct pollfd* pf, CheckType type)
	{
		assert(pf != nullptr);
		if (type == CheckRead && (pf->revents & CHECK_READ_FLAG))
			return true;
		if (type == CheckWrite && (pf->revents & CHECK_WRITE_FLAG))
			return true;
		if (type == CheckError && (pf->revents & CHECK_ERROR_FLAG))
			return true;

		return false;
	}

    bool ConnectorContext::CheckConnectSuccess(int fd , bool check_write)
    {   
		auto pf = FindPollfd(fd);
		if (nullptr == pf)
			return false;
        if(check_write && !CheckEvent(pf, CheckWrite))
            return false; 
        int err;
        int errlen = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t*)&errlen) == -1) {
            return false;
        }

        return err == 0 ; 
    }

	int ConnectorContext::StartPoll(int timeout)
	{
		if (pollfds_.empty())
			return 0;
		int ret = ::poll(&pollfds_[0], pollfds_.size(), timeout);
		if (ret == -1)
			ret = errno != EINTR ? -1 : 0;

		return ret;
	}

    void ConnectorContext::RunOnceCheckConnect(int millsecond)
    {
        if(StartPoll(millsecond) < 0)
            return ; 
        std::vector<struct pollfd> result ;
		for (auto& item : pollfds_) {
			if (CheckEvent(&item, CheckWrite)) 
				result.push_back(item);
		}
        std::set<int> total_fds ; 
        std::set<int> success_fds ;
        for(auto& item : result) {
            total_fds.insert(item.fd);
            // 判断连接是否成功
            if(CheckConnectSuccess(item.fd , false) && !CheckSelfConnect(item.fd))
                success_fds.insert(item.fd);
        }

        for(auto fd  : total_fds)
        {
			auto iter = std::find_if(pollfds_.begin(), pollfds_.end(), [fd](struct pollfd& self)->bool {
				return self.fd == fd;
			});
			if (iter != pollfds_.end())
				pollfds_.erase(iter);
            auto it = connecting_tasks_.find(fd) ;
            assert(it != connecting_tasks_.end());
            if(it == connecting_tasks_.end())
                continue ; 
            else {
                const auto& info = it->second; 
                if(success_fds.find(fd) != success_fds.end()) {
                    auto tcp_socket = TcpSocket::Create(fd); 
                    if(info.server_addr_->success_callback() != nullptr)
                        info.server_addr_->success_callback()(std::move(tcp_socket)) ;
                }
                else
                {
                    if(info.server_addr_->failed_callback() != nullptr)
                        info.server_addr_->failed_callback()();
                }
            }
			connecting_tasks_.erase(it) ;
        }
    }

    bool ConnectorContext::CheckSelfConnect(int fd)
    {
        struct sockaddr_in local_addr{0} ,  remote_addr{0};
        socklen_t addr_len = static_cast<socklen_t>(sizeof(local_addr));
        ::getsockname(fd , (struct sockaddr *)(&local_addr) , &addr_len) ; 
        ::getpeername(fd , (struct sockaddr *)(&remote_addr) , &addr_len) ;   

        return local_addr.sin_port == remote_addr.sin_port && 
                local_addr.sin_addr.s_addr == remote_addr.sin_addr.s_addr ; 
    }

    void ConnectorContext::ProcessConnectTask(const ConnectAddress::Ptr& addr)
    {
        struct sockaddr_in server_addr = {0};
        spdnet::base::InitSocketEnv();
        int client_fd = socket(PF_INET , SOCK_STREAM , 0 ) ; 
        if(client_fd == -1)
            return ; 
        spdnet::base::SetSocketNonBlock(client_fd) ; 
        server_addr.sin_family = AF_INET ; 
        inet_pton(AF_INET , addr->ip().c_str() , &server_addr.sin_addr.s_addr);
        server_addr.sin_port = htons(addr->port());

        int ret = ::connect(client_fd , (struct sockaddr*)&server_addr , sizeof(struct sockaddr)) ; 
        if(ret == 0) {
            // 连接成功
            if(CheckSelfConnect(client_fd))
                goto FAILED ;
        }
        else if(errno != EINPROGRESS) {
            goto FAILED ; 
        }
        else {
			 ConnectingTask ci ;
             ci.start_connect_time_ = std::chrono::steady_clock::now();
             ci.server_addr_ = addr ; 
			 connecting_tasks_[client_fd] = ci ;
             // 加入poll
             JoinPoll(client_fd , CheckWrite) ; 
             return ; 
        }

        if(addr->success_callback() != nullptr)
        {
            auto tcp_socket = TcpSocket::Create(client_fd); 
            addr->success_callback()(std::move(tcp_socket)); 
        }

        return ; 
    FAILED:
        if(client_fd != -1) {
            spdnet::base::CloseSocket(client_fd) ; 
            client_fd = -1;
        }
        if(addr->failed_callback() != nullptr) {
            addr->failed_callback()(); 
        }
    }

    class AsyncConnector::ConnectOptions::Options
    {
     public:
        std::string ip_ ; 
        int port_ ; 
        AsyncConnector::SuccessCallback success_callback_ ; 
        AsyncConnector::FailedCallback  failed_callback_ ; 
    } ; 

    AsyncConnector::ConnectOptions::ConnectOptionsFunc AsyncConnector::ConnectOptions::WithAddr(const std::string& ip , int port)
    {
        return [ip , port](AsyncConnector::ConnectOptions::Options& option){
            option.ip_ = ip ; 
            option.port_ = port ; 
        } ; 
    }

    AsyncConnector::ConnectOptions::ConnectOptionsFunc AsyncConnector::ConnectOptions::WithSuccessCallback(SuccessCallback&& callback)
    {
        return [callback](AsyncConnector::ConnectOptions::Options& option){
            option.success_callback_ = callback ; 
        } ; 
    }


    AsyncConnector::ConnectOptions::ConnectOptionsFunc AsyncConnector::ConnectOptions::WithFailedCallback(FailedCallback&& callback)
    {
        return [callback](AsyncConnector::ConnectOptions::Options& option){ 
            option.failed_callback_ = callback ; 
        } ; 
    }


    AsyncConnector::AsyncConnector()
        :context_(std::make_shared<ConnectorContext>())
    {

    }

    AsyncConnector::~AsyncConnector()
    {

    }

    void AsyncConnector::AsyncConnect(const std::vector<ConnectOptions::ConnectOptionsFunc>& options)
    {
        ConnectOptions::Options option ; 
        for(auto& func : options)
            func(option) ; 
        auto address = std::shared_ptr<ConnectAddress>(new ConnectAddress(std::move(option.ip_) , option.port_ , std::move(option.success_callback_ ), std::move(option.failed_callback_)));
        auto shared_this = shared_from_this();
        auto context = context_;
        std::lock_guard<std::mutex> lock(async_tasks_guard_);
        async_tasks_.emplace_back([context, address](){
            context->ProcessConnectTask(address) ; 
        });
    }

    void AsyncConnector::ProcessAyncTask()
    {
        std::vector<AsyncTask>  tmp ; 
        {
            std::lock_guard<std::mutex> lock(async_tasks_guard_);
            tmp.swap(async_tasks_);
        }

        for(auto& task : tmp)
        {
            task()  ; 
        }
    }

    void AsyncConnector::StartWorkerThread()
    {   
        is_run_ = std::make_shared<bool>(true);
        auto is_run = is_run_ ; 
        auto context = context_;
        auto shared_this = shared_from_this();
        thread_ = std::make_shared<std::thread>([is_run , context, shared_this](){
            while (*is_run)
            {
				context->RunOnceCheckConnect(0) ;
                shared_this->ProcessAyncTask();
                std::this_thread::sleep_for(std::chrono::milliseconds(10)) ; 
            }
            

        });
    }

    AsyncConnector::Ptr AsyncConnector::Create()
    {
        class make_shared_enabler : public AsyncConnector {};
        return std::make_shared<make_shared_enabler>() ; 
    }



}
}