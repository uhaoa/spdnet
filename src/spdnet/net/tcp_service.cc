#include <spdnet/net/tcp_service.h>
#include <iostream>
#include <spdnet/base/socket_api.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/tcp_connection.h>

namespace spdnet
{
namespace net
{
	TcpService::TcpService()noexcept
         :listen_epoll_fd_(epoll_create(1)),
         random_(time(0))
    {

    }

	TcpService::~TcpService() noexcept
    {

    } 

    EventLoop::Ptr TcpService::GetRandomEventLoop()
    {
        auto rand_num = random_();
        return loops[rand_num % loops.size()] ; 
    }

    void TcpService::AddTcpConnection(TcpSocket::Ptr tcp_socket , const EventLoop::TcpEnterCallback& enter_callback)
    {
        EventLoop::Ptr loop = GetRandomEventLoop();
        TcpConnection::Ptr new_connection = TcpConnection::Create(std::move(tcp_socket) , loop); 
        auto task = std::bind(&EventLoop::OnTcpConnectionEnter , loop , new_connection , enter_callback) ; 
        loop->RunInEventLoop(task) ; 
    }

    const static unsigned int kDefaultLoopTimeout = 100 ; 

    void TcpService::LaunchWorkerThread(size_t num)
    {
        if(num <= 0)
            throw SpdCommonException(std::string("thread_num must > 0 "));

        run_loop_ = std::make_shared<bool>(true);
        loops.resize(num);
        for(auto& loop : loops)
        {
            loop = EventLoop::Create(kDefaultLoopTimeout);
            loop->Run(run_loop_);
        }

    }

    void TcpService::StartServer(const std::string& ip , int port  , std::shared_ptr<ServiceParam> params)
    {
        std::lock_guard<std::mutex> service_guard(service_lock_);

        ip_ = ip ; 
        port_ = port ; 
		params_ = std::move(params);

        const int fd = Listen();
        if(-1 == fd)  {
            throw SpdCommonException(std::string("listen error : ")  + std::to_string(errno));
        }
        struct  epoll_event ev ; 
        ev.events = EPOLLIN ; 
        ev.data.ptr = nullptr ; 
        if(epoll_ctl(listen_epoll_fd_ , EPOLL_CTL_ADD , fd , &ev) != 0){
            throw SpdCommonException(std::string("epoll_ctl error : ")  + std::to_string(errno));
        }
        auto listen_socket = std::shared_ptr<ListenSocket>(ListenSocket::Create(fd)) ; 
        listen_socket->SetNonblock();
        
        run_listen_  = std::make_shared<bool>(true);
        auto is_run  = run_listen_ ;  
        auto param   = params_ ;
        auto service = shared_from_this();
        auto epoll_fd = listen_epoll_fd_ ; 
        listen_thread_ = std::make_shared<std::thread>([is_run , listen_socket , epoll_fd , param , service ]() mutable
            {
                while(*is_run)
                {
                    struct epoll_event ev ; 
                    int ret = epoll_wait(epoll_fd , &ev , 1 , 1000); 
                    if(ret == 1 && ev.events & EPOLLIN){
                        auto tcp_socket = listen_socket->Accept() ; 
                        if(nullptr != tcp_socket){
                            if(param->nodelay())
                                tcp_socket->SetNoDelay(); 
                            if(tcp_socket->SetNonblock()) 
                                service->AddTcpConnection(std::move(tcp_socket) , param->enter_callback()); 
                        }
                    }

                }
            }
        ) ; 

    }

    void TcpService::Stop()
    {
        std::lock_guard<std::mutex> service_guard(service_lock_);
    
        try
        {
            if(run_listen_)
            {
                *run_listen_ = false ; 
                if(listen_thread_->joinable())
                    listen_thread_->join();
                listen_thread_ = nullptr ; 
            }
            *run_loop_ = false;
            for (auto& loop : loops)
            {
                if(loop->GetLoopThread()->joinable())
                    loop->GetLoopThread()->join();
            }

            loops.clear();
            
        }
        catch(std::system_error& err)
        {
            (void)err ; 
        }



    }

	TcpService::Ptr TcpService::Create()
    {
        return std::make_shared<TcpService>(); 
    }

	int TcpService::Listen()
    {
        base::InitSocketEnv();

        struct sockaddr_in addr{0};
		int fd = socket(AF_INET , SOCK_STREAM , 0) ;
        if(fd == -1){
            return  -1 ; 
        }
        
        addr.sin_family = AF_INET ; 
        addr.sin_port = htons(port_) ; 
        addr.sin_addr.s_addr = INADDR_ANY; 
        if(inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) == 0){
            base::CloseSocket(fd) ; 
            return  -1;
        }
        int reuse_on = 1 ; 
        if(setsockopt(fd , SOL_SOCKET , SO_REUSEADDR , (const char*)&reuse_on  , sizeof(int)) < 0){
            base::CloseSocket(fd) ; 
            return  -1;
        }
        
        int ret = bind(fd , (struct sockaddr*)&addr , sizeof(addr)) ;
        if(ret == -1 || listen(fd , 512) == -1){
            base::CloseSocket(fd) ; 
            return -1 ; 
        }

        return fd ; 

    }
}
}

