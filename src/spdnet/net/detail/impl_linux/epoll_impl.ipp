#ifndef SPDNET_NET_EPOLL_IMPL_IPP_
#define SPDNET_NET_EPOLL_IMPL_IPP_

#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#include <memory>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <sys/epoll.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/task_executor.h>
#include <spdnet/net/detail/impl_linux/epoll_socket_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
            EpollImpl::EpollImpl(std::shared_ptr<TaskExecutor> task_executor,
                                 std::function<void(sock_t)> &&socket_close_notify_cb)
                    : epoll_fd_(::epoll_create(1)),
                      task_executor_(task_executor),
                      socket_close_notify_cb_(socket_close_notify_cb) {
                linkChannel(wakeup_.eventfd(), &wakeup_, EPOLLET | EPOLLIN | EPOLLRDHUP);
                event_entries_.resize(1024);
            }

            EpollImpl::~EpollImpl() noexcept {
                ::close(epoll_fd_);
                epoll_fd_ = -1;
            }

            void EpollImpl::addWriteEvent(SocketData::Ptr data) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
                event.data.ptr = data->channel_.get();
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, data->sock_fd(), &event);
            }

            void EpollImpl::cancelWriteEvent(SocketData::Ptr data) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
                event.data.ptr = data->channel_.get();
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, data->sock_fd(), &event);
            }

            bool EpollImpl::linkChannel(int fd, const Channel *channel, uint32_t events) {
                struct epoll_event event{0, {nullptr}};
                event.events = events;
                event.data.ptr = (void *) (channel);
                return ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
            }

            void EpollImpl::send(SocketData::Ptr socket_data, const char *data, size_t len) {
                if (!socket_data->is_can_write_)
                    return;
                auto buffer = spdnet::base::BufferPool::instance().allocBufferBySize(len);
                assert(buffer);
                buffer->write(data, len);
                {
                    std::lock_guard<spdnet::base::SpinLock> lck(socket_data->send_guard_);
                    socket_data->send_buffer_list_.push_back(buffer);
                }
                if (socket_data->is_post_flush_) {
                    return;
                }
                socket_data->is_post_flush_ = true;

                task_executor_->post([socket_data]() {
                    if (socket_data->is_can_write_) {
                        socket_data->channel_->flushBuffer();
                        socket_data->is_post_flush_ = false;
                    }
                });
            }

            bool EpollImpl::startAccept(sock_t listen_fd, const Channel *channel) {
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.ptr = (void *) channel;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd, &ev) != 0) {
                    return false;
                }

                return true;
            }

            bool EpollImpl::asyncConnect(sock_t client_fd, const EndPoint &addr, Channel *channel) {
                int ret = ::connect(client_fd, addr.socket_addr(), addr.socket_addr_len());
                if (ret == 0) {
                    channel->trySend();
                    return true;
                } else if (errno != EINPROGRESS) {
                    channel->onClose();
                    return true;
                }
                return linkChannel(client_fd, channel, EPOLLET | EPOLLOUT | EPOLLRDHUP);
            }

            void EpollImpl::wakeup() {
                wakeup_.wakeup();
            }

            void EpollImpl::closeSocket(SocketData::Ptr data) {
                if (data->has_closed_)
                    return;

                // closeSocket函数可能正在被channel调用 ， 将channel加入到待删除列表 ，是防止channel被立即释放引起crash
                del_channel_list_.emplace_back(data->channel_);

                // cancel event
                struct epoll_event ev{0, {nullptr}};
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, data->sock_fd(), &ev);

                socket_close_notify_cb_(data->sock_fd());

                data->close();
            }

            void EpollImpl::shutdownSocket(SocketData::Ptr data) {
                if (data->has_closed_)
                    return;
                ::shutdown(data->sock_fd(), SHUT_WR);
                data->is_can_write_ = false;
            }

            bool EpollImpl::onSocketEnter(SocketData::Ptr data) {
                auto impl = shared_from_this();
                data->channel_ = std::make_shared<EPollSocketChannel>(impl, data);
                return linkChannel(data->sock_fd(), data->channel_.get(), EPOLLET | EPOLLIN | EPOLLRDHUP);
            }


            void EpollImpl::runOnce(uint32_t timeout) {
                int num_events = ::epoll_wait(epoll_fd_, event_entries_.data(), event_entries_.size(), timeout);
                for (int i = 0; i < num_events; i++) {
                    auto channel = static_cast<Channel *>(event_entries_[i].data.ptr);
                    auto event = event_entries_[i].events;

                    if (SPDNET_PREDICT_FALSE(event & EPOLLRDHUP)) {
                        channel->tryRecv();
                        channel->onClose();
                        continue;
                    }
                    if (SPDNET_PREDICT_TRUE(event & EPOLLIN)) {
                        channel->tryRecv();
                    }
                    if (event & EPOLLOUT) {
                        channel->trySend();
                    }
                }

                if (SPDNET_PREDICT_FALSE(num_events == static_cast<int>(event_entries_.size()))) {
                    event_entries_.resize(event_entries_.size() * 2);
                }

                // release channel 
                del_channel_list_.clear();
            }
        }
    }
}
#endif //SPDNET_NET_EPOLL_IMPL_IPP_