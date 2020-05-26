#ifndef SPDNET_NET_EVENT_LOOP_H_
#define SPDNET_NET_EVENT_LOOP_H_
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/base.h>
#include <spdnet/base/current_thread.h>
#include <spdnet/net/tcp_connection.h>

namespace spdnet
{
namespace net
{

class Channel; 
class WakeupChannel;
class EventLoop : public base::NonCopyable
{
    public:
        using Ptr = std::shared_ptr<EventLoop> ; 
        using AsynLoopTask = std::function<void()> ;     
        using AfterLoopTask = std::function<void()>; 
    public: 
        EventLoop(unsigned int)noexcept; 
        virtual ~EventLoop()noexcept;
        void run(std::shared_ptr<bool>);
        static Ptr create(unsigned int loop_timeout_ms);
        inline bool   isInLoopThread()const
        {
            return thread_id_ == current_thread::tid();    
        }

        void runInEventLoop(AsynLoopTask&& task);
        void runAfterEventLoop(AfterLoopTask&& task);

        bool linkChannel(int fd , const Channel* channel ,  uint32_t events = EPOLLET | EPOLLIN | EPOLLRDHUP);
        void onTcpConnectionEnter(TcpConnection::Ptr tcp_connection , const TcpConnection::TcpEnterCallback& enter_callback);
        TcpConnection::Ptr getTcpConnection(int fd) ;
        void addTcpConnection(TcpConnection::Ptr);
        void removeTcpConnection(int fd);
        const std::shared_ptr<std::thread>& getLoopThread()const {
            return loop_thread_ ; 
        } 
        int epoll_fd()const{
            return epoll_fd_;
        }
    private:
        void execAsyncTasks();
        void execAfterTasks();

    private:
        int epoll_fd_ ;
		int							  thread_id_ ;
        std::mutex                    task_mutex_;
        std::vector<AsynLoopTask>     async_tasks ;
        std::vector<AsynLoopTask>     tmp_async_tasks ; 
        std::vector<AfterLoopTask>    after_loop_tasks ;
        std::vector<AfterLoopTask>    tmp_after_loop_tasks ; 
        std::shared_ptr<std::thread>  loop_thread_ ; 
        unsigned int                  wait_timeout_ms_ ; 
        std::vector<epoll_event>      event_entries_ ; 
        std::unique_ptr<WakeupChannel>      wake_up_ ; 
        std::unordered_map<int, TcpConnection::Ptr> tcp_connections_ ;
} ; 

}
}
#endif  // SPDNET_NET_EVENT_LOOP_H_