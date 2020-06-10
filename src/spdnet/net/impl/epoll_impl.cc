#include <memory>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <sys/epoll.h>
#include <spdnet/net/impl/epoll_impl.h>
#include <spdnet/base/current_thread.h>
#include <spdnet/net/channel.h>
#include <spdnet/net/exception.h>

namespace spdnet {
	namespace net {
		namespace impl {
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

			bool EpollImpl::linkEvent(int fd, const Channel* channel, uint32_t events) {
				struct epoll_event event { 0, { nullptr } };
				event.events = events;
				event.data.ptr = (void*)(channel);
				return ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
			}

			void EpollImpl::runOnce() {
				int num_events = ::epoll_wait(epoll_fd_, event_entries_.data(), event_entries_.size(), wait_timeout_ms_);
				for (int i = 0; i < num_events; i++) {
					auto channel = static_cast<Channel*>(event_entries_[i].data.ptr);
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