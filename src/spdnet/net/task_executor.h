#ifndef SPDNET_NET_TASK_EXECUTOR_H
#define SPDNET_NET_TASK_EXECUTOR_H

#include <iostream>
#include <functional>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/wakeup_base.h>

namespace spdnet {
    namespace net {
        using task_functor = std::function<void()>;

        class task_executor : public spdnet::base::noncopyable {
        public:
            task_executor(wakeup_base *wakeup)
                    : wakeup_(wakeup) {

            }

            void post(task_functor &&task, bool is_try_immediate = true) {
                if (is_try_immediate && is_in_io_thread()) {
                    // immediate exec
                    task();
                } else {
                    if (!is_in_io_thread()) {
                        std::lock_guard<std::mutex> lck(task_mutex_);
                        async_tasks.emplace_back(std::move(task));
                    } else {
                        sync_tasks.emplace_back(std::move(task));
                    }
                    wakeup_->wakeup();
                }
            }

            void run() {
                // 执行其它线程投递的task
                {
                    std::lock_guard<std::mutex> lck(task_mutex_);
                    tmp_async_tasks.swap(async_tasks);
                }
                for (auto &task : tmp_async_tasks) {
                    task();
                }
                tmp_async_tasks.clear();
                // ִ执行本线程投递的task
                tmp_sync_tasks.swap(sync_tasks);
                for (auto &task : tmp_sync_tasks) {
                    task();
                }
                tmp_sync_tasks.clear();
            }

            void set_thread_id(thread_id_t id) {
                thread_id_ = id;
            }

            bool is_in_io_thread() const {
                return current_thread::tid() == thread_id_;
            }

        private:
            thread_id_t thread_id_{0};
            std::mutex task_mutex_;
            std::vector<task_functor> async_tasks;
            std::vector<task_functor> sync_tasks;
            std::vector<task_functor> tmp_async_tasks;
            std::vector<task_functor> tmp_sync_tasks;
            wakeup_base *wakeup_{nullptr};
        };
    }
}
#endif // SPDNET_NET_TASK_EXECUTOR_H
