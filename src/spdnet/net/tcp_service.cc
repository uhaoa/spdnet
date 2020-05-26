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
    const static unsigned int kDefaultLoopTimeout = 100 ; 

	TcpService::TcpService()noexcept
         :random_(time(0))
    {

    }

	TcpService::~TcpService() noexcept
    {
        stop() ; 
    } 

    EventLoop::Ptr TcpService::getEventLoop()
    {
        auto rand_num = random_();
        return loops_[rand_num % loops_.size()] ; 
    }

    void TcpService::addTcpConnection(std::shared_ptr<TcpSocket> tcp_socket , const TcpConnection::TcpEnterCallback& enter_callback)
    {
        EventLoop::Ptr loop = getEventLoop();
        TcpConnection::Ptr new_connection = TcpConnection::create(std::move(tcp_socket) , loop); 
		loop->runInEventLoop([loop ,new_connection , enter_callback]() {
			loop->onTcpConnectionEnter(new_connection, enter_callback);
		}); 
    }

    void TcpService::runThread(size_t thread_num)
    {
        if(thread_num <= 0)
            throw SpdnetException(std::string("thread_num must > 0 "));

        run_loop_ = std::make_shared<bool>(true);
        for(size_t i = 0 ; i < thread_num ; i++) {
            auto loop = EventLoop::create(kDefaultLoopTimeout);
            loop->run(run_loop_);
            loops_.push_back(loop); 
        }
    }

    void TcpService::stop()
    {
        try
        {
            if (run_loop_)
            {
                *run_loop_ = false;
                for (auto& loop : loops_)
                {
                    if(loop->getLoopThread()->joinable())
                        loop->getLoopThread()->join();
                }

                loops_.clear();
            }
        }
        catch(std::system_error& err)
        {
            (void)err ; 
        }



    }
}
}

