#ifndef SPDNET_NET_EVENT_LOOP_H_
#define SPDNET_NET_EVENT_LOOP_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/base.h>
#include <spdnet/base/current_thread.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/base/buffer_pool.h>

namespace spdnet::net {
    using namespace base;

    class Channel;

    class WakeupChannel;

    class EventLoop : public base::NonCopyable {
    public:
        using Ptr = std::shared_ptr<EventLoop>;
        using AsynLoopTask = std::function<void()>;
        using AfterLoopTask = std::function<void()>;
    public:
        explicit EventLoop(unsigned int) noexcept;

        virtual ~EventLoop() noexcept;

        void run(std::shared_ptr<bool>);

        static Ptr create(unsigned int loop_timeout_ms);

        inline bool isInLoopThread() const {
            return thread_id_ == current_thread::tid();
        }

        void runInEventLoop(AsynLoopTask &&task);

        void runAfterEventLoop(AfterLoopTask &&task);

        bool linkChannel(int fd, const Channel *channel, uint32_t events = EPOLLET | EPOLLIN | EPOLLRDHUP);

        void onTcpSessionEnter(TcpSession::Ptr tcp_session, const TcpSession::TcpEnterCallback &enter_callback);

        TcpSession::Ptr getTcpSession(int fd);

        void addTcpSession(TcpSession::Ptr);

        void removeTcpSession(int fd);

        const std::shared_ptr<std::thread> &getLoopThread() const {
            return loop_thread_;
        }

        int epoll_fd() const {
            return epoll_fd_;
        }

        Buffer *getBufferBySize(size_t size) {
            return buffer_pool_.getBufferBySize(size);
        }

        void releaseBuffer(Buffer *buffer) {
            buffer_pool_.releaseBuffer(buffer);
        }

    private:
        void execAsyncTasks();

        void execAfterTasks();

    private:
        int epoll_fd_;
        int thread_id_;
        std::mutex task_mutex_;
        std::vector<AsynLoopTask> async_tasks;
        std::vector<AsynLoopTask> tmp_async_tasks;
        std::vector<AfterLoopTask> after_loop_tasks;
        std::vector<AfterLoopTask> tmp_after_loop_tasks;
        std::shared_ptr<std::thread> loop_thread_;
        unsigned int wait_timeout_ms_;
        std::vector<epoll_event> event_entries_;
        std::unique_ptr<WakeupChannel> wake_up_;
        std::unordered_map<int, TcpSession::Ptr> tcp_sessions_;
        BufferPool buffer_pool_;
    };

}
#endif  // SPDNET_NET_EVENT_LOOP_H_