#ifndef SPDNET_NET_SERVICE_THREAD_IPP_
#define SPDNET_NET_SERVICE_THREAD_IPP_

#include <memory>
#include <iostream>
#include <cassert>
#include <spdnet/net/service_thread.h>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
    namespace net {

        ServiceThread::ServiceThread(unsigned int wait_timeout_ms)
                : wait_timeout_ms_(wait_timeout_ms) {
            task_executor_ = std::make_shared<TaskExecutor>(this);
            auto impl = std::make_shared<detail::IoObjectImplType>(task_executor_, [this](sock_t fd) {
                removeTcpSession(fd);
            });
            io_impl_ = impl;
        }


        void ServiceThread::post(AsynTaskFunctor &&task) {
            task_executor_->post(std::move(task));
        }

        std::shared_ptr<TcpSession> ServiceThread::getTcpSession(sock_t fd) {
            auto iter = tcp_sessions_.find(fd);
            if (iter != tcp_sessions_.end())
                return iter->second;
            else
                return nullptr;
        }

        void ServiceThread::addTcpSession(sock_t fd, std::shared_ptr<TcpSession> session) {
            tcp_sessions_[fd] = std::move(session);
        }

        void ServiceThread::removeTcpSession(sock_t fd) {
            auto iter = tcp_sessions_.find(fd);
            if (iter != tcp_sessions_.end()) {
                tcp_sessions_.erase(iter);
            } else {
                assert(false);
            }
        }

        void
        ServiceThread::onTcpSessionEnter(sock_t fd, std::shared_ptr<TcpSession> tcp_session,
                                         const TcpEnterCallback &enter_callback) {
            if (!io_impl_->onSocketEnter(tcp_session->socket_data_)) {
                return;
            }
            if (nullptr != enter_callback)
                enter_callback(tcp_session);

            addTcpSession(fd, std::move(tcp_session));
        }

        void ServiceThread::run(std::shared_ptr<bool> is_run) {
            thread_ = std::make_shared<std::thread>([is_run, this]() {
                thread_id_ = current_thread::tid();
                task_executor_->setThreadId(thread_id_);
                while (*is_run) {
                    // run io
                    io_impl_->runOnce(wait_timeout_ms_);
                    // do task
                    task_executor_->run();
                    // release channel
                    io_impl_->releaseChannel();
                }
            });
        }
    }
}

#endif // SPDNET_NET_SERVICE_THREAD_IPP_