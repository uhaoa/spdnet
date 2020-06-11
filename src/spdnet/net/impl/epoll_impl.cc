#include <memory>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <sys/epoll.h>
#include <spdnet/net/impl/epoll_impl.h>
#include <spdnet/base/current_thread.h>
#include <spdnet/net/channel.h>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session.h>

namespace spdnet {
	namespace net {

			class EpollImpl::TcpSessionChannel : public Channel
			{
			public:
			    using Ptr = TcpSessionChannel*;
				EpollImpl::TcpSessionChannel(TcpSession::Ptr session , EpollImpl::Ptr impl)
					: session_(session) , impl_ptr_(impl)
				{

				}
				void trySend() override {
					impl_ptr_->flushBuffer(session_); 
				}

				void tryRecv() override {
					impl_ptr_->recv(session_); 
				}

				void onClose() override {
					impl_ptr_->close(session);
				}
			private:
				TcpSession::Ptr session_; 
				EpollImpl::Ptr impl_ptr_;
			};

			struct EpollImpl::AcceptorChannel :public Channel
			{

			};

			struct EpollImpl::ConnectorChannel : public Channel
			{

			};

			struct EpollImpl::SocketData : public base::NonCopyable
            {
                SocketData(TcpSession::Ptr session , EpollImpl::Ptr impl)
                    :channel_(session , impl)
                {

                }
                Buffer recv_buffer_;
                std::deque<Buffer*> send_buffer_list_;
                std::deque<Buffer*> pending_buffer_list_;
                SpinLock send_guard_;
                volatile bool has_closed_{ false };
                volatile bool is_post_flush_{ false };
                volatile bool is_can_write_{ true };
                TcpSessionChannel channel_{nullptr}
            };

			EpollImpl::EpollImpl(unsigned int wait_timeout_ms) noexcept
				: epoll_fd_(::epoll_create(1)),
				wait_timeout_ms_(wait_timeout_ms) {
				auto event_fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
				wake_up_.reset(new WakeupChannel(event_fd));
				linkEvent(event_fd, wake_up_.get());
				event_entries_.resize(1024);
			}

			EpollImpl::~EpollImpl() noexcept {
				::close(epoll_fd_);
				epoll_fd_ = -1;
			}

			EpollImpl::Ptr EpollImpl::create(unsigned int wait_timeout_ms) {
				return std::make_shared<EpollImpl>(wait_timeout_ms);
			}

            void EpollImpl::recycle_socket_data(void* ptr)
            {
                auto socket_data = (SocketData*)ptr;
                delete socket_data;
            }


            bool EpollImpl::linkEvent(int fd, const Channel* channel, uint32_t events) {
				struct epoll_event event { 0, { nullptr } };
				event.events = events;
				event.data.ptr = (void*)(channel);
				return ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
			}

            void EpollImpl::send(TcpSession::Ptr session) {
                if (session_ptr->desciptor_data().is_post_flush_) {
                    return;
                }
                session->desciptor_data().is_post_flush_ = true;
                loop_owner_->runInEventLoop([session_ptr = session , this]() {
                    if (session_ptr->desciptor_data().is_can_write_) {
                        this->flushBuffer(session_ptr);
                        session_ptr->desciptor_data().is_post_flush_ = false;
                    }
                });
            }

            void EpollImpl::regWriteEvent(TcpSession::Ptr session) {
                struct epoll_event event{0, {   nullptr}};
                event.events = EPOLLET | EPOLLIN | EPOLLOUT | EPOLLRDHUP;
                event.data.ptr = session.get();
                ::epoll_ctl(loop_owner_->epoll_fd(), EPOLL_CTL_MOD, data.fd_, &event);
            }

            void EpollImpl::close(TcpSession::Ptr session)
            {
                auto& data = session->desciptor_data();
                if (data.has_closed_)
                    return;
                data.has_closed_ = true;
                data.is_can_write_ = false;
                loop_owner_->runAfterEventLoop([loop, callback, session, socket]() {
                    if (callback)
                        callback(session);
                    loop->removeTcpSession(socket->sock_fd());
                    struct epoll_event ev{0, {nullptr}};
                    ::epoll_ctl(loop->epoll_fd(), EPOLL_CTL_DEL, socket->sock_fd(), &ev);
                });
                session->onClose();
            }

            void EpollImpl::onSessionEnter(TcpSession::Ptr session)
            {
				TcpSessionChannel* channel = new TcpSessionChannel(session , this);
				linkEvent(session->getSocketFd(), channel, EPOLLET | EPOLLIN | EPOLLRDHUP); 
            }

            void EpollImpl::flushBuffer(TcpSession::Ptr session)
            {
                auto& data = session->desciptor_data();
                bool force_close = false;
                assert(loop_owner_->isInLoopThread());
                {
                    std::lock_guard<SpinLock> lck(data.send_guard_);
                    if (SPDNET_PREDICT_TRUE(data.pending_buffer_list_.empty())) {
                        data.pending_buffer_list_.swap(data.send_buffer_list_);
                    } else {
                        for (const auto buffer : data.send_buffer_list_)
                            data.pending_buffer_list_.push_back(buffer);
                        data.send_buffer_list_.clear();
                    }
                }

                constexpr size_t MAX_IOVEC = 1024;
                struct iovec iov[MAX_IOVEC];
                while (!data.pending_buffer_list_.empty()) {
                    size_t cnt = 0;
                    size_t prepare_send_len = 0;
                    for (const auto buffer : data.pending_buffer_list_) {
                        iov[cnt].iov_base = buffer->getDataPtr();
                        iov[cnt].iov_len = buffer->getLength();
                        prepare_send_len += buffer->getLength();
                        cnt++;
                        if (cnt >= MAX_IOVEC)
                            break;
                    }
                    assert(cnt > 0);
                    const int send_len = ::writev(data.fd_, iov, static_cast<int>(cnt));
                    if (SPDNET_PREDICT_FALSE(send_len < 0)) {
                        if (errno == EAGAIN) {
                            regWriteEvent(session);
                            data.is_can_write_ = false;
                        } else {
                            force_close = true;
                        }
                    } else {
                        size_t tmp_len = send_len;
                        for (auto iter = data.pending_buffer_list_.begin(); iter != data.pending_buffer_list_.end();) {
                            auto buffer = *iter;
                            if (SPDNET_PREDICT_TRUE(buffer->getLength() <= tmp_len)) {
                                tmp_len -= buffer->getLength();
                                buffer->clear();
                                loop_owner_->releaseBuffer(buffer);
                                iter = data.pending_buffer_list_.erase(iter);
                            } else {
                                buffer->removeLength(tmp_len);
                                break;
                            }

                        }
                    }
                }

                if (force_close) {
                    close(session);
                }
            }

            void EpollImpl::recv(TcpSession::Ptr session)
            {
				auto& data = session->desciptor_data();
				char stack_buffer[65536];
				bool force_close = false;
				auto& recv_buffer = data.recv_buffer_;
				while (true) {
					size_t valid_count = recv_buffer.getWriteValidCount();
					struct iovec vec[2];
					vec[0].iov_base = recv_buffer.getWritePtr();
					vec[0].iov_len = valid_count;
					vec[1].iov_base = stack_buffer;
					vec[1].iov_len = sizeof(stack_buffer);


					size_t try_recv_len = valid_count + sizeof(stack_buffer);

					int recv_len = static_cast<int>(::readv(data.fd_, vec, 2));
					if (SPDNET_PREDICT_FALSE(recv_len == 0 || (recv_len < 0 && errno != EAGAIN))) {
						force_close = true;
						break;
					}
					size_t stack_len = 0;
					if (SPDNET_PREDICT_FALSE(recv_len > (int) valid_count)) {
						recv_buffer.addWritePos(valid_count);
						stack_len = recv_len - valid_count;
					}
					else
						recv_buffer.addWritePos(recv_len);

					if (SPDNET_PREDICT_TRUE(nullptr != data.data_callback_)) {
						size_t len = data.data_callback_(recv_buffer.getDataPtr(), recv_buffer.getLength());
						assert(len <= recv_buffer.getLength());
						if (SPDNET_PREDICT_TRUE(len == recv_buffer.getLength())) {
							recv_buffer.removeLength(len);
							if (stack_len > 0) {
								len = data.data_callback_(stack_buffer, stack_len);
								assert(len <= stack_len);
								if (len < stack_len) {
									recv_buffer.write(stack_buffer + len, stack_len - len);
								}
								else if (len > stack_len) {
									force_close = true;
									break;
								}
							}
						}
						else if (len < recv_buffer.getLength()) {
							recv_buffer.removeLength(len);
							if (stack_len > 0)
								recv_buffer.write(stack_buffer, stack_len);
						}
						else {
							force_close = true;
							break;
						}

					}

					if (SPDNET_PREDICT_FALSE(
						recv_len >= (int)recv_buffer.getCapacity()/* + (int)(sizeof(stack_buffer))*/)) {
						size_t grow_len = 0;
						if (recv_buffer.getCapacity() * 2 <= max_recv_buffer_size_)
							grow_len = recv_buffer.getCapacity();
						else
							grow_len = max_recv_buffer_size_ - recv_buffer.getCapacity();

						if (grow_len > 0)
							recv_buffer_.grow(grow_len);
					}

					if (SPDNET_PREDICT_FALSE(recv_buffer.getWriteValidCount() == 0 || recv_buffer.getLength() == 0))
						recv_buffer.adjustToHead();

					if (recv_len < static_cast<int>(try_recv_len))
						break;
				}

				if (force_close)
					close(session);
            }

			void EpollImpl::runOnce() {
				int num_events = ::epoll_wait(epoll_fd_, event_entries_.data(), event_entries_.size(), wait_timeout_ms_);
				for (int i = 0; i < num_events; i++) {
					auto ptr = static_cast<session*>(event_entries_[i].data.ptr);
                    auto session = ptr->shared_from_this();
					auto event = event_entries_[i].events;

					if (SPDNET_PREDICT_FALSE(event & EPOLLRDHUP)) {
						recv(session);
                        close(session);
						continue;
					}
					if (SPDNET_PREDICT_TRUE(event & EPOLLIN)) {
                        recv(session);
					}
					if (event & EPOLLOUT) {
						send(session);
					}
				}

				if (SPDNET_PREDICT_FALSE(num_events == static_cast<int>(event_entries_.size()))) {
					event_entries_.resize(event_entries_.size() * 2);
				}

			}
	}
}