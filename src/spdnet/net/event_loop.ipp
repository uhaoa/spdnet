#ifndef SPDNET_NET_EVENT_LOOP_IPP_
#define SPDNET_NET_EVENT_LOOP_IPP_

#include <memory>
#include <iostream>
#include <cassert>
#include <spdnet/net/event_loop.h>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/exception.h>
#ifdef SPDNET_PLATFORM_LINUX
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#else
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#endif


namespace spdnet {
    namespace net {

        EventLoop::EventLoop(unsigned int wait_timeout_ms)
                : wait_timeout_ms_(wait_timeout_ms) 
		{
			task_executor_ = std::make_shared<TaskExecutor>(); 
			io_impl_ = std::make_shared<IoObjectImplType>(task_executor_, [this](sock_t fd) {
				removeTcpSession(fd); 
			});
        }


		void EventLoop::post(AsynTaskFunctor&& task) {
			task_executor_->post(std::move(task)); 
		}

		std::shared_ptr<TcpSession> EventLoop::getTcpSession(sock_t fd) {
            auto iter = tcp_sessions_.find(fd);
            if (iter != tcp_sessions_.end())
                return iter->second;
            else
                return nullptr;
        }

        void EventLoop::addTcpSession(std::shared_ptr<TcpSession> session) {
            tcp_sessions_[session->sock_fd()] = std::move(session);
        }

        void EventLoop::removeTcpSession(sock_t fd) {
            auto iter = tcp_sessions_.find(fd);
            if (iter != tcp_sessions_.end()) {
                tcp_sessions_.erase(iter);
            } else {
                assert(false);
            }
        }

        void
        EventLoop::onTcpSessionEnter(std::shared_ptr<TcpSession> tcp_session, const TcpEnterCallback &enter_callback) {
            assert(isInLoopThread());
            if (!io_impl_->onSocketEnter(*tcp_session->socket_data_)) {
                return;
            }
            assert(nullptr == getTcpSession(tcp_session->sock_fd()));
            if (nullptr != enter_callback)
                enter_callback(tcp_session);

            addTcpSession(std::move(tcp_session));
        }

        void EventLoop::run(std::shared_ptr<bool> is_run) {
            loop_thread_ = std::make_shared<std::thread>([is_run, this]() {
                thread_id_ = current_thread::tid();
				task_executor_->setThreadId(thread_id_); 
                while (*is_run) {
					// run io 
                    io_impl_->runOnce(wait_timeout_ms_);
					// do task
					task_executor_->run(); 
                }
            });
        }
    }
}

#endif // SPDNET_NET_EVENT_LOOP_IPP_