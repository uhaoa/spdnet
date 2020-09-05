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
#include <spdnet/net/tcp_session_mgr.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/detail/impl_linux/epoll_socket_channel.h>
#if defined(SPDNET_USE_OPENSSL)
#include <spdnet/net/detail/ssl/epoll_ssl_channel.h>
#endif


namespace spdnet {
    namespace net {
        namespace detail {
            epoll_impl::epoll_impl(std::shared_ptr<task_executor> task_executor,
                                   std::shared_ptr<channel_collector> channel_collector)
                    : epoll_fd_(::epoll_create(1)),
                      task_executor_(task_executor),
                      channel_collector_(channel_collector) {
                link_channel(wakeup_.eventfd(), &wakeup_, EPOLLET | EPOLLIN | EPOLLRDHUP);
                event_entries_.resize(1024);
            }

            epoll_impl::~epoll_impl()  {
                ::close(epoll_fd_);
                epoll_fd_ = -1;
            }

            void epoll_impl::add_write_event(std::shared_ptr<tcp_session> session) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
                event.data.ptr = session->channel_.get();
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, session->sock_fd(), &event);
            }

            void epoll_impl::cancel_write_event(std::shared_ptr<tcp_session> session) {
                struct epoll_event event{0, {nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
                event.data.ptr = session->channel_.get();
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, session->sock_fd(), &event);
            }

            bool epoll_impl::link_channel(int fd, const channel *ch, uint32_t events) {
                struct epoll_event event{0, {nullptr}};
                event.events = events;
                event.data.ptr = (void *) (ch);
                return ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
            }

            void epoll_impl::post_flush(tcp_session* session) {
                task_executor_->post([session]() {
                    if (session->is_can_write_) {
						session->channel_->flush_buffer();
                    }
					session->is_post_flush_ = false;
                }, false);
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

            void epoll_impl::close_socket(std::shared_ptr<tcp_session> session) {
                if (session->has_closed_)
                    return;

                // close_socket函数可能正在被channel调用 ， 将channel加入到待删除列表 ，是防止channel被立即释放引起crash
                channel_collector_->put_channel(session->channel_);
                // cancel event
                struct epoll_event ev{0, {nullptr}};
                ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, session->sock_fd(), &ev);

				tcp_session_mgr::instance().remove(session->sock_fd()); 

                session->close();
            }

            void epoll_impl::shutdown_session(std::shared_ptr<tcp_session> session) {
                if (session->has_closed_)
                    return;
                ::shutdown(session->sock_fd(), SHUT_WR);
				session->is_can_write_ = false;
            }

            bool epoll_impl::on_socket_enter(std::shared_ptr<tcp_session> session) {
                auto impl = shared_from_this();
                if (session->ssl_context_ == nullptr) {
                    session->channel_ = std::make_shared<epoll_socket_channel>(impl, session);
                    return link_channel(session->sock_fd(), session->channel_.get(), EPOLLET | EPOLLIN | EPOLLRDHUP);
                }
                else {
#if defined(SPDNET_USE_OPENSSL)
                    session->ssl_channel_ = std::make_shared<epoll_ssl_channel>(impl, session);
                    session->ssl_context_->try_start_ssl_handshake();
                    return link_channel(session->sock_fd(), session->ssl_channel_.get(), EPOLLET | EPOLLIN | EPOLLRDHUP);
#endif
                }
               
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