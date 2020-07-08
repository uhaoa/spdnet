#ifndef SPDNET_NET_TASK_EXECUTOR_H
#define SPDNET_NET_TASK_EXECUTOR_H

#include <iostream>
#include <functional>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/wakeup_base.h>

namespace spdnet {
    namespace net {
        using AsynTaskFunctor = std::function<void()>;

        class TaskExecutor : public spdnet::base::NonCopyable {
        public:
            TaskExecutor(WakeupBase *wakeup)
                    : wakeup_(wakeup) {

            }

			void post(AsynTaskFunctor&& task, bool is_try_immediate = true) {
                if (is_try_immediate && current_thread::tid() == thread_id_) {
                    // immediate exec
                    task();
                } else {
                    {
                        std::lock_guard<std::mutex> lck(task_mutex_);
						async_tasks.emplace_back(std::move(task));
                    }
                    wakeup_->wakeup();
                }
            }

            void run() {
                {
                    std::lock_guard<std::mutex> lck(task_mutex_);
                    tmp_async_tasks.swap(async_tasks);
                }
                for (auto &task : tmp_async_tasks) {
                    task();
                }
                tmp_async_tasks.clear();
            }

            void setThreadId(thread_id_t id) {
                thread_id_ = id;
            }

        private:
            thread_id_t thread_id_;
            std::mutex task_mutex_;
            std::vector<AsynTaskFunctor> async_tasks;
            std::vector<AsynTaskFunctor> tmp_async_tasks;
            WakeupBase *wakeup_;
        };
    }
}
#endif // SPDNET_NET_TASK_EXECUTOR_H
