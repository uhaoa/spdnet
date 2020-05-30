#ifndef SPDNET_NET_EVENTSERVICE_H_
#define SPDNET_NET_EVENTSERVICE_H_
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
    class EventService : public base::NonCopyable
    {
     public:

	EventService() noexcept;
        ~EventService() noexcept;

	void addTcpSession(std::shared_ptr<TcpSocket> tcp_socket , const TcpSession::TcpEnterCallback& enter_callback);
	void runThread(size_t thread_num);
	EventLoop::Ptr getEventLoop();
     private:
	void stop();
     private:
        std::shared_ptr<bool>				   run_loop_ ; 
        std::vector<EventLoop::Ptr>            loops_ ; 
        std::mt19937                           random_ ; 
    } ; 
}
}


#endif // SPDNET_NET_EVENTSERVICE_H_
