#include <memory>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <sys/epoll.h>
#include <spdnet/net/event_loop.h>
#include <spdnet/base/current_thread.h>
#include <spdnet/net/exception.h>

namespace spdnet {
    namespace net {

        EventLoop::EventLoop(unsigned int wait_timeout_ms) noexcept
                :wait_timeout_ms_(wait_timeout_ms) {
            event_entries_.resize(1024);
            io_impl_.reset(new EpollImpl(*this));
        }

        EventLoop::~EventLoop() noexcept {
            close(epoll_fd_);
            epoll_fd_ = -1;
        }

        void EventLoop::runInEventLoop(AsynLoopTask &&task) {
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

        void EventLoop::runAfterEventLoop(AfterLoopTask &&task) {
            assert(isInLoopThread());
            after_loop_tasks.emplace_back(std::move(task));
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

        void EventLoop::execAfterTasks() {
            tmp_after_loop_tasks.swap(after_loop_tasks);
            for (auto &task : tmp_after_loop_tasks) {
                task();
            }
            tmp_after_loop_tasks.clear();
        }

        TcpSession::Ptr EventLoop::getTcpSession(int fd) {
            auto iter = tcp_sessions_.find(fd);
            if (iter != tcp_sessions_.end())
                return iter->second;
            else
                return nullptr;
        }

        void EventLoop::addTcpSession(TcpSession::Ptr connection) {
            tcp_sessions_[connection->getSocketFd()] = std::move(connection);
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
            io_impl_.onSessionEnter(tcp_session); 
            assert(nullptr == getTcpSession(tcp_session->getSocketFd()));
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
                    execAfterTasks();

                }
            });
        }
    }
}