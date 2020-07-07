#ifndef SPDNET_NET_EPOLL_IMPL_H_
#define SPDNET_NET_EPOLL_IMPL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <iostream>
#include <unordered_map>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/base/buffer.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/end_point.h>
#include <spdnet/net/detail/impl_linux/epoll_wakeup_channel.h>

namespace spdnet {
    namespace net {
        class TaskExecutor;

        class AsyncConnector;
        namespace detail {
            class Channel;

            class EpollImpl : public spdnet::base::NonCopyable, public std::enable_shared_from_this<EpollImpl> {
            public:
                friend class EPollSocketChannel;

                explicit EpollImpl(std::shared_ptr<TaskExecutor> task_executor,
                                   std::function<void(sock_t)> &&socket_close_notify_cb);

                virtual ~EpollImpl() noexcept;

                bool onSocketEnter(SocketData::Ptr data);

                void runOnce(uint32_t timeout);

                void send(SocketData* socket_data, const char *data, size_t len);

                void shutdownSocket(SocketData::Ptr data);

                int epoll_fd() const { return epoll_fd_; }

                bool linkChannel(int fd, const Channel *channel, uint32_t events);

                bool startAccept(sock_t listen_fd, const Channel *channel);

                bool asyncConnect(sock_t client_fd, const EndPoint &addr, Channel *channel);

                void wakeup();

				spdnet::base::Buffer* allocBufferBySize(size_t size) {
					return buffer_pool_.allocBufferBySize(size); 
				}

				void recycleBuffer(spdnet::base::Buffer* buffer)
				{
					buffer_pool_.recycleBuffer(buffer); 
				}

				void releaseChannel()
                {
                    del_channel_list_.clear();
                }
            private:
                void closeSocket(SocketData::Ptr data);

                void addWriteEvent(SocketData::Ptr data);

                void cancelWriteEvent(SocketData::Ptr data);

                void doRecv(SocketData::Ptr data);

            private:
                int epoll_fd_;
                EpollWakeupChannel wakeup_;
				spdnet::base::BufferPool buffer_pool_; 
                std::vector<epoll_event> event_entries_;
                std::shared_ptr<TaskExecutor> task_executor_;
                std::function<void(sock_t)> socket_close_notify_cb_;
                std::vector<std::shared_ptr<Channel>> del_channel_list_;
            };

            using IoObjectImplType = EpollImpl;
        }
    }
}

#include <spdnet/net/detail/impl_linux/epoll_impl.ipp>

#endif  // SPDNET_NET_EPOLL_IMPL_H_