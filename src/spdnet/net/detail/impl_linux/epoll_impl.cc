#include <memory>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <sys/epoll.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/event_loop.h>
#include <spdnet/net/detail/impl_linux/channel.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>

namespace spdnet {
    namespace net {
        namespace detail {
            EpollImpl::EpollImpl(EventLoop &loop_owner) noexcept
                    : epoll_fd_(::epoll_create(1)), loop_ref_(loop_owner) {
                auto event_fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
                wake_up_.reset(new WakeupChannel(event_fd));
                linkChannel(event_fd, wake_up_.get(), EPOLLET | EPOLLIN | EPOLLRDHUP);
                event_entries_.resize(1024);
            }

            EpollImpl::~EpollImpl() noexcept {
                ::close(epoll_fd_);
                epoll_fd_ = -1;
            }

			EpollImpl::SocketImplData::SocketImplData(EpollImpl& impl, std::shared_ptr<TcpSocket> socket)
				:socket_(std::move(socket))
			{
				channel_ = std::make_shared<TcpSessionChannel>(impl, *this);
			}

            void EpollImpl::wakeup() {
                wake_up_->wakeup();
            }

            void EpollImpl::addWriteEvent(SocketImplData& socket_data) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
                event.data.ptr = &socket_data.channel_;
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, socket_data.sock_fd(), &event);
            }

            void EpollImpl::cancelWriteEvent(SocketImplData& socket_data) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
                event.data.ptr = &socket_data.channel_;
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, socket_data.socket_->sock_fd(), &event);
            }

            bool EpollImpl::linkChannel(int fd, const Channel *channel, uint32_t events) {
                struct epoll_event event{0, {nullptr}};
                event.events = events;
                event.data.ptr = (void *) (channel);
                return ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
            }

            void EpollImpl::send(SocketImplData& socket_data, const char* data, size_t len) {
				if (!socket_data.is_can_write_)
					return; 
				auto buffer = loop_ref_.allocBufferBySize(len);
				assert(buffer);
				buffer->write(data, len);
				{
					std::lock_guard<SpinLock> lck(socket_data.send_guard_);
                    socket_data.send_buffer_list_.push_back(buffer);
				}
                if (socket_data.is_post_flush_) {
                    return;
                }
				socket_data.is_post_flush_ = true;
				auto& this_ref = *this;
				loop_ref_.post([&socket_data, &this_ref]() {
                    if (socket_data.is_can_write_) {
						this_ref.flushBuffer(socket_data);
						socket_data.is_post_flush_ = false;
                    }
                });
            }

            void EpollImpl::closeSocket(SocketImplData& socket_data) {
				assert(loop_ref_.isInLoopThread());
                if (socket_data.has_closed_)
                    return;
				socket_data.has_closed_ = true;
				socket_data.is_can_write_ = false;
				auto& loop = loop_ref_;
				auto sockfd = socket_data.socket_->sock_fd();
				// remove session
				loop_ref_.post([&loop , sockfd](){
                    loop.postToNextCircle([&loop , sockfd](){
                        loop.removeTcpSession(sockfd);
                    });
				});

				// cancel event
                struct epoll_event ev{0, {nullptr}};
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket_data.socket_->sock_fd(), &ev);

				if (socket_data.disconnect_callback_)
					socket_data.disconnect_callback_();
            }

            void EpollImpl::shutdownSocket(SocketImplData& socket_data) {
                if (socket_data.has_closed_)
                    return;
                assert(loop_ref_.isInLoopThread());
                ::shutdown(socket_data.socket_->sock_fd(), SHUT_WR);
                socket_data.is_can_write_ = false;
            }

            void EpollImpl::onSocketEnter(SocketImplData& socket_data) {
                linkChannel(socket_data.socket_->sock_fd(), socket_data.channel_.get(), EPOLLET | EPOLLIN | EPOLLRDHUP);
            }

            void EpollImpl::flushBuffer(SocketImplData& socket_data) {
                bool force_close = false;
                assert(loop_ref_.isInLoopThread());
                {
                    std::lock_guard<SpinLock> lck(socket_data.send_guard_);
                    if (SPDNET_PREDICT_TRUE(socket_data.pending_buffer_list_.empty())) {
						socket_data.pending_buffer_list_.swap(socket_data.send_buffer_list_);
                    } else {
                        for (const auto buffer : socket_data.send_buffer_list_)
							socket_data.pending_buffer_list_.push_back(buffer);
						socket_data.send_buffer_list_.clear();
                    }
                }

                constexpr size_t MAX_IOVEC = 1024;
                struct iovec iov[MAX_IOVEC];
                while (!socket_data.pending_buffer_list_.empty()) {
                    size_t cnt = 0;
                    size_t prepare_send_len = 0;
                    for (const auto buffer : socket_data.pending_buffer_list_) {
                        iov[cnt].iov_base = buffer->getDataPtr();
                        iov[cnt].iov_len = buffer->getLength();
                        prepare_send_len += buffer->getLength();
                        cnt++;
                        if (cnt >= MAX_IOVEC)
                            break;
                    }
                    assert(cnt > 0);
                    const int send_len = ::writev(socket_data.sock_fd(), iov, static_cast<int>(cnt));
                    if (SPDNET_PREDICT_FALSE(send_len < 0)) {
                        if (errno == EAGAIN) {
                            addWriteEvent(socket_data);
							socket_data.is_can_write_ = false;
                        } else {
                            force_close = true;
                        }
                    } else {
                        size_t tmp_len = send_len;
                        for (auto iter = socket_data.pending_buffer_list_.begin();
                             iter != socket_data.pending_buffer_list_.end();) {
                            auto buffer = *iter;
                            if (SPDNET_PREDICT_TRUE(buffer->getLength() <= tmp_len)) {
                                tmp_len -= buffer->getLength();
                                buffer->clear();
								loop_ref_.recycleBuffer(buffer);
                                iter = socket_data.pending_buffer_list_.erase(iter);
                            } else {
                                buffer->removeLength(tmp_len);
                                break;
                            }

                        }
                    }
                }

                if (force_close) {
                    closeSocket(socket_data);
                }
            }

            void EpollImpl::doRecv(SocketImplData& socket_data) {
                char stack_buffer[65536];
                bool force_close = false;
                auto &recv_buffer = socket_data.recv_buffer_;
                while (true) {
                    size_t valid_count = recv_buffer.getWriteValidCount();
                    struct iovec vec[2];
                    vec[0].iov_base = recv_buffer.getWritePtr();
                    vec[0].iov_len = valid_count;
                    vec[1].iov_base = stack_buffer;
                    vec[1].iov_len = sizeof(stack_buffer);


                    size_t try_recv_len = valid_count + sizeof(stack_buffer);

                    int recv_len = static_cast<int>(::readv(socket_data.socket_->sock_fd(), vec, 2));
                    if (SPDNET_PREDICT_FALSE(recv_len == 0 || (recv_len < 0 && errno != EAGAIN))) {
                        force_close = true;
                        break;
                    }
                    size_t stack_len = 0;
                    if (SPDNET_PREDICT_FALSE(recv_len > (int) valid_count)) {
                        recv_buffer.addWritePos(valid_count);
                        stack_len = recv_len - valid_count;
                    } else
                        recv_buffer.addWritePos(recv_len);

                    if (SPDNET_PREDICT_TRUE(nullptr != socket_data.data_callback_)) {
                        size_t len = socket_data.data_callback_(recv_buffer.getDataPtr(), recv_buffer.getLength());
                        assert(len <= recv_buffer.getLength());
                        if (SPDNET_PREDICT_TRUE(len == recv_buffer.getLength())) {
                            recv_buffer.removeLength(len);
                            if (stack_len > 0) {
                                len = socket_data.data_callback_(stack_buffer, stack_len);
                                assert(len <= stack_len);
                                if (len < stack_len) {
                                    recv_buffer.write(stack_buffer + len, stack_len - len);
                                } else if (len > stack_len) {
                                    force_close = true;
                                    break;
                                }
                            }
                        } else if (len < recv_buffer.getLength()) {
                            recv_buffer.removeLength(len);
                            if (stack_len > 0)
                                recv_buffer.write(stack_buffer, stack_len);
                        } else {
                            force_close = true;
                            break;
                        }

                    }

                    if (SPDNET_PREDICT_FALSE(
                            recv_len >= (int) recv_buffer.getCapacity()/* + (int)(sizeof(stack_buffer))*/)) {
                        size_t grow_len = 0;
                        if (recv_buffer.getCapacity() * 2 <= socket_data.max_recv_buffer_size_)
                            grow_len = recv_buffer.getCapacity();
                        else
                            grow_len = socket_data.max_recv_buffer_size_ - recv_buffer.getCapacity();

                        if (grow_len > 0)
                            recv_buffer.grow(grow_len);
                    }

                    if (SPDNET_PREDICT_FALSE(recv_buffer.getWriteValidCount() == 0 || recv_buffer.getLength() == 0))
                        recv_buffer.adjustToHead();

                    if (recv_len < static_cast<int>(try_recv_len))
                        break;
                }

                if (force_close)
                    closeSocket(socket_data);
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
            }
        }
    }
}