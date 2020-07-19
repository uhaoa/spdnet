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
            epoll_impl::epoll_impl(std::shared_ptr<task_executor> task_executor,
                                 std::shared_ptr<channel_collector> channel_collector,
                                 std::function<void(sock_t)> &&socket_close_notify_cb)
                    : epoll_fd_(::epoll_create(1)),
                      task_executor_(task_executor),
                      channel_collector_(channel_collector),
                      socket_close_notify_cb_(socket_close_notify_cb) {
                link_channel(wakeup_.eventfd(), &wakeup_, EPOLLET | EPOLLIN | EPOLLRDHUP);
                event_entries_.resize(1024);
            }

            epoll_impl::~epoll_impl() noexcept {
                ::close(epoll_fd_);
                epoll_fd_ = -1;
            }

            void epoll_impl::add_write_event(socket_data::ptr data) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
                event.data.ptr = data->channel_.get();
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, data->sock_fd(), &event);
            }

            void epoll_impl::cancel_write_event(socket_data::ptr data) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
                event.data.ptr = data->channel_.get();
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, data->sock_fd(), &event);
            }

            bool epoll_impl::link_channel(int fd, const channel *ch, uint32_t events) {
                struct epoll_event event{0, {nullptr}};
                event.events = events;
                event.data.ptr = (void *) (ch);
                return ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
            }

            void epoll_impl::send(socket_data *socket_data, const char *data, size_t len) {
                auto buffer = alloc_buffer(len);
                assert(buffer);
                buffer->write(data, len);
                {
                    std::lock_guard<spdnet::base::spin_lock> lck(socket_data->send_guard_);
                    socket_data->send_buffer_list_.push_back(buffer);
                }
                if (socket_data->is_post_flush_) {
                    return;
                }
                socket_data->is_post_flush_ = true;

                task_executor_->post([socket_data]() {
                    if (socket_data->is_can_write_) {
                        socket_data->channel_->flush_buffer();
                    }
                    socket_data->is_post_flush_ = false;
                } , false);
            }

            bool epoll_impl::start_accept(sock_t listen_fd, const channel *ch) {
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.ptr = (void *) ch;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd, &ev) != 0) {
                    return false;
                }

                return true;
            }

            bool epoll_impl::async_connect(sock_t client_fd, const end_point &addr, channel *ch) {
                int ret = ::connect(client_fd, addr.socket_addr(), addr.socket_addr_len());
                if (ret == 0) {
                    ch->on_send();
                    return true;
                } else if (errno != EINPROGRESS) {
                    ch->on_close();
                    return true;
                }
                return link_channel(client_fd, ch, EPOLLET | EPOLLOUT | EPOLLRDHUP);
            }

            void epoll_impl::wakeup() {
                wakeup_.wakeup();
            }

            void epoll_impl::close_socket(socket_data::ptr data) {
                if (data->has_closed_)
                    return;

                // close_socket函数可能正在被channel调用 ， 将channel加入到待删除列表 ，是防止channel被立即释放引起crash
                channel_collector_->put_channel(data->channel_);
                // cancel event
                struct epoll_event ev{0, {nullptr}};
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, data->sock_fd(), &ev);

                socket_close_notify_cb_(data->sock_fd());

                data->close();
            }

            void epoll_impl::shutdown_socket(socket_data::ptr data) {
                if (data->has_closed_)
                    return;
                ::shutdown(data->sock_fd(), SHUT_WR);
                data->is_can_write_ = false;
            }

            bool epoll_impl::on_socket_enter(socket_data::ptr data) {
                auto impl = shared_from_this();
                data->channel_ = std::make_shared<epoll_socket_channel>(impl, data);
                return link_channel(data->sock_fd(), data->channel_.get(), EPOLLET | EPOLLIN | EPOLLRDHUP);
            }


            void epoll_impl::run_once(uint32_t timeout) {
                int num_events = ::epoll_wait(epoll_fd_, event_entries_.data(), event_entries_.size(), timeout);
                for (int i = 0; i < num_events; i++) {
                    auto ch = static_cast<channel *>(event_entries_[i].data.ptr);
                    auto event = event_entries_[i].events;

                    if (SPDNET_PREDICT_FALSE(event & EPOLLRDHUP)) {
                        ch->on_recv();
                        ch->on_close();
                        continue;
                    }
                    if (SPDNET_PREDICT_TRUE(event & EPOLLIN)) {
                        ch->on_recv();
                    }
                    if (event & EPOLLOUT) {
                        ch->on_send();
                    }
                }

                if (SPDNET_PREDICT_FALSE(num_events == static_cast<int>(event_entries_.size()))) {
                    event_entries_.resize(event_entries_.size() * 2);
                }

                // release channel
                // del_channel_list_.clear();
            }
        }
    }
}
#endif //SPDNET_NET_EPOLL_IMPL_IPP_