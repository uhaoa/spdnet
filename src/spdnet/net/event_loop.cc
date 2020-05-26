#include <memory>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <sys/epoll.h>
#include <spdnet/net/event_loop.h>
#include <spdnet/base/current_thread.h>
#include <spdnet/net/channel.h>
#include <spdnet/net/exception.h>

namespace spdnet
{
namespace net
{

    EventLoop::EventLoop(unsigned int wait_timeout_ms)noexcept
        : epoll_fd_(epoll_create(1)) , 
          wait_timeout_ms_(wait_timeout_ms)
    {
        auto event_fd = ::eventfd(0 , EFD_CLOEXEC | EFD_NONBLOCK );
        wake_up_.reset(new WakeupChannel(event_fd));
        linkChannel(event_fd , wake_up_.get());
        event_entries_.resize(1024) ; 
    }

    EventLoop::~EventLoop()noexcept
    {
        close(epoll_fd_); 
        epoll_fd_ = -1 ;  
    }

    EventLoop::Ptr EventLoop::create(unsigned int wait_timeout_ms)
    {
        return std::make_shared<EventLoop>(wait_timeout_ms);
    }

    void EventLoop::runInEventLoop(AsynLoopTask&& task)
    {
        if(isInLoopThread()){
            task();
        }
        else
        {
			{
				std::lock_guard<std::mutex> lck(task_mutex_);
				async_tasks.emplace_back(std::move(task));
			}
            // wakeup 
            wake_up_->wakeup();
        }
        
    }

    void EventLoop::runAfterEventLoop(AfterLoopTask&& task)
    {
        assert(isInLoopThread());
        if(!isInLoopThread())
        {
            throw SpdnetException("call runAfterEventLoop() must in the loop thread!");
        }
        after_loop_tasks.emplace_back(std::move(task));
    }

    void EventLoop::execAsyncTasks()
    {
        {
            std::lock_guard<std::mutex> lck(task_mutex_);
            tmp_async_tasks.swap(async_tasks);
        }
        for(auto& task : tmp_async_tasks) {
            task();
        }
        tmp_async_tasks.clear();
    }

	void EventLoop::execAfterTasks()
	{
		tmp_after_loop_tasks.swap(after_loop_tasks);
		for (auto& task : tmp_after_loop_tasks) {
			task();
		}
		tmp_after_loop_tasks.clear();
	}

    TcpConnection::Ptr EventLoop::getTcpConnection(int fd)
    {
        auto iter = tcp_connections_.find(fd);
        if(iter != tcp_connections_.end())
            return iter->second; 
        else 
            return nullptr ; 
    }

    void EventLoop::addTcpConnection(TcpConnection::Ptr connection)
    {
        tcp_connections_[connection->getSocketFd()] = std::move(connection);
    }

    void EventLoop::removeTcpConnection(int fd)
    {
        auto iter = tcp_connections_.find(fd);
        if(iter != tcp_connections_.end())
        {
            tcp_connections_.erase(iter);
        }   
        else{
            assert(false);
        }
    }

    void EventLoop::onTcpConnectionEnter(TcpConnection::Ptr tcp_connection , const TcpConnection::TcpEnterCallback& enter_callback)
    {
        assert(isInLoopThread());

        if(!linkChannel(tcp_connection->getSocketFd() , tcp_connection.get())) {
            // error
            return ; 
        }
        assert(nullptr == getTcpConnection(tcp_connection->getSocketFd()));
        if(nullptr != enter_callback)
            enter_callback(tcp_connection);

        addTcpConnection(std::move(tcp_connection));
    }

    bool EventLoop::linkChannel(int fd , const Channel* channel , uint32_t events)
    {
        struct epoll_event event { 0  , {nullptr}} ;
		event.events = events; 
        event.data.ptr = (void*)(channel) ;
        return ::epoll_ctl(epoll_fd_ , EPOLL_CTL_ADD , fd , &event) == 0 ; 
    }

    void EventLoop::run(std::shared_ptr<bool> is_run)
    {
        loop_thread_ = std::make_shared<std::thread>([is_run , this]() {
            thread_id_ = current_thread::tid();
            while(*is_run)
            {
                auto timeout_ms = wait_timeout_ms_ ; 
                int num_events = epoll_wait(epoll_fd_ , event_entries_.data() , event_entries_.size() , timeout_ms) ; 

                for (int i = 0; i < num_events; i++)
                {
                    auto channel = static_cast<Channel*>(event_entries_[i].data.ptr);
                    auto event   = event_entries_[i].events ; 

                    if(event & EPOLLRDHUP)
                    {
                        channel->tryRecv();
                        channel->onClose();
                        continue;
                    }
                    if(event & EPOLLIN)
                    {
                        channel->tryRecv();     
                    }
                    if(event & EPOLLOUT)
                    {
                        channel->trySend();
                    }
                }

                if(num_events == static_cast<int>(event_entries_.size())){
                    event_entries_.resize(event_entries_.size() * 2); 
                }

                execAsyncTasks();
                execAfterTasks();
                
            }
        });
    }
}
}