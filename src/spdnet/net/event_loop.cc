#include <memory>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <sys/epoll.h>
#include <spdnet/net/event_loop.h>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>

namespace spdnet {
    namespace net {

        EventLoop::EventLoop(unsigned int wait_timeout_ms) noexcept
                : wait_timeout_ms_(wait_timeout_ms) {
            io_impl_.reset(new detail::EpollImpl(*this));
        }


        void EventLoop::post(AsynLoopTask &&task) {
            if (SPDNET_PREDICT_FALSE(isInLoopThread())) {
                task();
            } else {
                {
                    std::lock_guard<std::mutex> lck(task_mutex_);
                    async_tasks.emplace_back(std::move(task));
                }
                io_impl_->wakeup();
            }

        }

        void EventLoop::postToNextCircle(AsynLoopTask &&task)
        {
            {
                std::lock_guard<std::mutex> lck(task_mutex_);
                async_tasks.emplace_back(std::move(task));
            }
            io_impl_->wakeup();
        }

        void EventLoop::execAsyncTasks() {
            {
                std::lock_guard<std::mutex> lck(task_mutex_);
                tmp_async_tasks.swap(async_tasks);
            }
            for (auto &task : tmp_async_tasks) {
                task();
            }
            tmp_async_tasks.clear();
        }

        TcpSession::Ptr EventLoop::getTcpSession(int fd) {
            auto iter = tcp_sessions_.find(fd);
            if (iter != tcp_sessions_.end())
                return iter->second;
            else
                return nullptr;
        }

        void EventLoop::addTcpSession(TcpSession::Ptr session) {
            tcp_sessions_[session->sock_fd()] = std::move(session);
        }

        void EventLoop::removeTcpSession(int fd) {
            auto iter = tcp_sessions_.find(fd);
            if (iter != tcp_sessions_.end()) {
                tcp_sessions_.erase(iter);
            } else {
                assert(false);
            }
        }

        void
        EventLoop::onTcpSessionEnter(TcpSession::Ptr tcp_session, const TcpSession::TcpEnterCallback &enter_callback) {
            assert(isInLoopThread());
            io_impl_->onTcpSessionEnter(*tcp_session);
            assert(nullptr == getTcpSession(tcp_session->sock_fd()));
            if (nullptr != enter_callback)
                enter_callback(tcp_session);

            addTcpSession(std::move(tcp_session));
        }

        void EventLoop::run(std::shared_ptr<bool> is_run) {
            loop_thread_ = std::make_shared<std::thread>([is_run, this]() {
                thread_id_ = current_thread::tid();
                while (*is_run) {
                    io_impl_->runOnce(wait_timeout_ms_);

                    execAsyncTasks();
                }
            });
        }
    }
}