#ifndef SPDNET_NET_EPOLL_SOCKET_CHANNEL_H_
#define SPDNET_NET_EPOLL_SOCKET_CHANNEL_H_

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_linux/epoll_impl.h>
#include <spdnet/net/detail/impl_linux/epoll_channel.h>
#include <spdnet/net/socket_data.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class EPollSocketChannel : public Channel {
            public:
                friend class EpollImpl;

                EPollSocketChannel(std::shared_ptr<EpollImpl> impl, SocketData::Ptr data)
                        : impl_(impl), data_(data) {

                }

                void flushBuffer() {
                    if (data_->has_closed_)
                        return;
                    bool force_close = false;
                    {
                        std::lock_guard<spdnet::base::SpinLock> lck(data_->send_guard_);
                        if (SPDNET_PREDICT_TRUE(data_->pending_buffer_list_.empty())) {
                            data_->pending_buffer_list_.swap(data_->send_buffer_list_);
                        } else {
                            for (const auto buffer : data_->send_buffer_list_)
                                data_->pending_buffer_list_.push_back(buffer);
                            data_->send_buffer_list_.clear();
                        }
                    }

                    constexpr size_t MAX_IOVEC = 1024;
                    struct iovec iov[MAX_IOVEC];
                    while (!data_->pending_buffer_list_.empty()) {
                        size_t cnt = 0;
                        size_t prepare_send_len = 0;
                        for (const auto buffer : data_->pending_buffer_list_) {
                            iov[cnt].iov_base = buffer->getDataPtr();
                            iov[cnt].iov_len = buffer->getLength();
                            prepare_send_len += buffer->getLength();
                            cnt++;
                            if (cnt >= MAX_IOVEC)
                                break;
                        }
                        assert(cnt > 0);
                        const int send_len = ::writev(data_->sock_fd(), iov, static_cast<int>(cnt));
                        if (SPDNET_PREDICT_FALSE(send_len < 0)) {
                            if (errno == EAGAIN) {
                                impl_->addWriteEvent(data_);
                                data_->is_can_write_ = false;
                            } else {
                                force_close = true;
                            }
                            break;
                        } else {
                            size_t tmp_len = send_len;
                            for (auto iter = data_->pending_buffer_list_.begin();
                                 iter != data_->pending_buffer_list_.end();) {
                                auto buffer = *iter;
                                if (SPDNET_PREDICT_TRUE(buffer->getLength() <= tmp_len)) {
                                    tmp_len -= buffer->getLength();
                                    buffer->clear();
                                    impl_->recycleBuffer(buffer);
                                    iter = data_->pending_buffer_list_.erase(iter);
                                } else {
                                    buffer->removeLength(tmp_len);
                                    break;
                                }

                            }
                        }
                    }

                    if (force_close) {
                        impl_->closeSocket(data_);
                    }
                }

            private:
                void trySend() override {
                    impl_->cancelWriteEvent(data_);
                    flushBuffer();
                }

                void tryRecv() override {
                    doRecv();
                }

                void onClose() override {
                    impl_->closeSocket(data_);
                }

                void doRecv() {
                    char stack_buffer[65536];
                    bool force_close = false;
                    auto &recv_buffer = data_->recv_buffer_;
                    while (true) {
                        size_t valid_count = recv_buffer.getWriteValidCount();
                        struct iovec vec[2];
                        vec[0].iov_base = recv_buffer.getWritePtr();
                        vec[0].iov_len = valid_count;
                        vec[1].iov_base = stack_buffer;
                        vec[1].iov_len = sizeof(stack_buffer);


                        size_t try_recv_len = valid_count + sizeof(stack_buffer);

                        int recv_len = static_cast<int>(::readv(data_->sock_fd(), vec, 2));
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

                        if (SPDNET_PREDICT_TRUE(nullptr != data_->data_callback_)) {
                            size_t len = data_->data_callback_(recv_buffer.getDataPtr(), recv_buffer.getLength());
                            assert(len <= recv_buffer.getLength());
                            if (SPDNET_PREDICT_TRUE(len == recv_buffer.getLength())) {
                                recv_buffer.removeLength(len);
                                if (stack_len > 0) {
                                    len = data_->data_callback_(stack_buffer, stack_len);
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
                            if (recv_buffer.getCapacity() * 2 <= data_->max_recv_buffer_size_)
                                grow_len = recv_buffer.getCapacity();
                            else
                                grow_len = data_->max_recv_buffer_size_ - recv_buffer.getCapacity();

                            if (grow_len > 0)
                                recv_buffer.grow(grow_len);
                        }

                        if (SPDNET_PREDICT_FALSE(recv_buffer.getWriteValidCount() == 0 || recv_buffer.getLength() == 0))
                            recv_buffer.adjustToHead();

                        if (recv_len < static_cast<int>(try_recv_len))
                            break;
                    }

                    if (force_close)
                        impl_->closeSocket(data_);
                }

            private:
                std::shared_ptr<EpollImpl> impl_;
                SocketData::Ptr data_;
            };

        }
    }

}

#endif  // SPDNET_NET_EPOLL_SOCKET_CHANNEL_H_