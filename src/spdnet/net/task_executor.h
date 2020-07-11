#ifndef SPDNET_NET_TASK_EXECUTOR_H
#define SPDNET_NET_TASK_EXECUTOR_H

#include <iostream>
#include <functional>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/wakeup_base.h>

namespace spdnet {
    namespace net {
        using TaskFunctor = std::function<void()>;

        class TaskExecutor : public spdnet::base::NonCopyable {
        public:
            TaskExecutor(WakeupBase *wakeup)
                    : wakeup_(wakeup) {

            }

            void post(TaskFunctor&&task, bool is_try_immediate = true) {
                if (is_try_immediate && isInIoThread()) {
                    // immediate exec
                    task();
                } else {
					if (!isInIoThread())
                    {
                        std::lock_guard<std::mutex> lck(task_mutex_);
                        async_tasks.emplace_back(std::move(task));
                    }
					else {
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
				for (auto& task : tmp_sync_tasks) {
					task();
				}
                tmp_sync_tasks.clear();
            }

            void setThreadId(thread_id_t id) {
                thread_id_ = id;
            }

            bool isInIoThread() const {
                return current_thread::tid() == thread_id_;
            }

        private:
			thread_id_t thread_id_{ 0 };
            std::mutex task_mutex_;
            std::vector<TaskFunctor> async_tasks;
            std::vector<TaskFunctor> sync_tasks;
            std::vector<TaskFunctor> tmp_async_tasks;
            std::vector<TaskFunctor> tmp_sync_tasks;
            WakeupBase *wakeup_{nullptr};
        };
    }
}
#endif // SPDNET_NET_TASK_EXECUTOR_H
