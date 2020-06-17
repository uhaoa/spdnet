#ifndef SPDNET_NET_EVENT_LOOP_H_
#define SPDNET_NET_EVENT_LOOP_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/current_thread.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/base/buffer_pool.h>

namespace spdnet::net {
    using namespace base;
    namespace detail {
        class EpollImpl;
    }

    class EventLoop : public base::NonCopyable {
    public:
        using AsynLoopTask = std::function<void()>;
#if defined SPDNET_PLATFORM_LINUX
		using IoImplType = detail::EpollImpl; 
#endif 
    public:
        explicit EventLoop(unsigned int) noexcept;

        ~EventLoop() = default;

        void run(std::shared_ptr<bool>);

        inline bool isInLoopThread() const {
            return thread_id_ == current_thread::tid();
        }

        void runInEventLoop(AsynLoopTask &&task);

        void onTcpSessionEnter(TcpSession::Ptr tcp_session, const TcpSession::TcpEnterCallback &enter_callback);

        TcpSession::Ptr getTcpSession(int fd);

        void addTcpSession(TcpSession::Ptr);

        void removeTcpSession(int fd);

        const std::shared_ptr<std::thread> &getLoopThread() const {
            return loop_thread_;
        }

        spdnet::base::Buffer *allocBufferBySize(size_t size) {
            return buffer_pool_.allocBufferBySize(size);
        }

        void recycleBuffer(spdnet::base::Buffer *buffer) {
            buffer_pool_.recycleBuffer(buffer);
        }

        detail::EpollImpl &getImpl() {
            return *io_impl_.get();
        }

    private:
        void execAsyncTasks();

    private:
        current_thread::THREAD_ID_TYPE thread_id_;
        std::shared_ptr<detail::EpollImpl> io_impl_;
        std::mutex task_mutex_;
        std::vector<AsynLoopTask> async_tasks;
        std::vector<AsynLoopTask> tmp_async_tasks;
        std::shared_ptr<std::thread> loop_thread_;
        unsigned int wait_timeout_ms_;
        std::unordered_map<int, TcpSession::Ptr> tcp_sessions_;
        spdnet::base::BufferPool buffer_pool_;
    };

}
#endif  // SPDNET_NET_EVENT_LOOP_H_