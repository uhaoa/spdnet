#ifndef SPDNET_NET_IOCP_IMPL_H_
#define SPDNET_NET_IOCP_IMPL_H_

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <iostream>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/base/buffer.h>
#include <spdnet/net/socket_data.h>
#include <spdnet/net/end_point.h>
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/detail/impl_win/iocp_wakeup_channel.h>
#include <spdnet/net/detail/impl_win/iocp_accept_channel.h>

namespace spdnet {
    namespace net {
        //ServiceThread
        class TaskExecutor;
        namespace detail {
            class Channel;

            class IocpRecieveChannel;

            class IocpSendChannel;

            class IocpImpl : public spdnet::base::NonCopyable, public std::enable_shared_from_this<IocpImpl> {
            public:
                friend class IocpRecvChannel;

                friend class IocpSendChannel;

                inline explicit IocpImpl(std::shared_ptr<TaskExecutor> task_executor,
                                         std::shared_ptr<ChannelCollector> channel_collector,
                                         std::function<void(sock_t)> &&socket_close_notify_cb) noexcept;

                inline virtual ~IocpImpl() noexcept;

                inline bool onSocketEnter(SocketData::Ptr data);

                inline void send(SocketData *ssocket_data, const char *data, size_t len);

                inline void runOnce(uint32_t timeout);

                inline bool startAccept(sock_t listen_fd, IocpAcceptChannel *channel);

                inline bool asyncConnect(sock_t fd, const EndPoint &addr, Channel *op);

                inline void shutdownSocket(SocketData::Ptr data);

                inline void wakeup();

                spdnet::base::Buffer *allocBufferBySize(size_t size) {
                    return buffer_pool_.allocBufferBySize(size);
                }

                void recycleBuffer(spdnet::base::Buffer *buffer) {
                    buffer_pool_.recycleBuffer(buffer);
                }

            private:
                inline void closeSocket(SocketData::Ptr data);

            private:
                HANDLE handle_;
                IocpWakeupChannel wakeup_op_;
                spdnet::base::BufferPool buffer_pool_;
                std::atomic<void *> connect_ex_{nullptr};
                std::shared_ptr<TaskExecutor> task_executor_;
                std::shared_ptr<ChannelCollector> channel_collector_;
                std::function<void(sock_t)> socket_close_notify_cb_;
            };

            using IoObjectImplType = IocpImpl;
        }
    }
}


#include <spdnet/net/detail/impl_win/iocp_impl.ipp>

#endif  // SPDNET_NET_IOCP_IMPL_H_