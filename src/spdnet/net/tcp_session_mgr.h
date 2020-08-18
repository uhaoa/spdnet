#ifndef SPDNET_NET_TCP_SESSION_MGR_H_
#define SPDNET_NET_TCP_SESSION_MGR_H_

#include <unordered_map>
#include <spdnet/base/platform.h>
#include <spdnet/base/singleton.h>

namespace spdnet {
    namespace net {
        class tcp_session_mgr : public spdnet::base::singleton<tcp_session_mgr> {
        public:
			/*
			std::shared_ptr<tcp_session> service_thread::get_tcp_session(sock_t fd) {
				auto iter = tcp_sessions_.find(fd);
				if (iter != tcp_sessions_.end())
					return iter->second;
				else
					return nullptr;
			}
			*/
			void add(sock_t fd, std::shared_ptr<tcp_session> session) {
				tcp_sessions_[fd] = std::move(session);
			}

			void remove(sock_t fd) {
				auto iter = tcp_sessions_.find(fd);
				if (iter != tcp_sessions_.end()) {
					tcp_sessions_.erase(iter);
				}
				else {
					assert(false);
				}
			}

        private:
			THREAD_LOCAL static std::unordered_map<sock_t, std::shared_ptr<tcp_session>> tcp_sessions_;
        };

		THREAD_LOCAL std::unordered_map<sock_t, std::shared_ptr<tcp_session>> tcp_session_mgr::tcp_sessions_;
    }

}
#endif  // SPDNET_NET_TCP_SESSION_MGR_H_