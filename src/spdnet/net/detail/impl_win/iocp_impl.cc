#include <spdnet/base/platform.h>
#ifdef SPDNET_PLATFORM_WINDOWS
#include <memory>
#include <iostream>
#include <cassert>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/event_loop.h>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>

namespace spdnet {
    namespace net {
		namespace detail {
			SocketImplData::SocketImplData(std::shared_ptr<EventLoop> loop, sock_t fd , bool is_server_side)
				:SocketDataBase(fd , is_server_side)
			{
				recv_channel_ = std::make_shared<SocketRecieveChannel>(*this, loop);
				send_channel_ = std::make_shared<SocketSendChannel>(*this, loop);
			}

			IocpImpl::IocpImpl(EventLoop& loop) noexcept
				: handle_(CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1)), loop_ref_(loop)
			{
				wakeup_op_ = std::make_shared<SocketWakeupChannel>();
			}

			IocpImpl::~IocpImpl() noexcept {
				CloseHandle(handle_);
				handle_ = INVALID_HANDLE_VALUE;
			}

			bool IocpImpl::onSocketEnter(SocketImplData& socket_data) {
				if (socket_data.isServerSide()) {
					if (CreateIoCompletionPort((HANDLE)socket_data.sock_fd(), handle_, 0, 0) == 0) {
						return false;
					}
				}
				startRecv(socket_data);
				return true;
			}

			bool IocpImpl::startAccept(sock_t listen_fd , Channel* op)
			{
				if (::CreateIoCompletionPort((HANDLE)listen_fd, handle_, 0, 0) == 0)
				{
					return false;
				}

				return true;
			}

			void IocpImpl::closeSocket(SocketImplData& socket_data)
			{
				assert(loop_ref_.isInLoopThread());
				if (socket_data.has_closed_)
					return;
				socket_data.has_closed_ = true;
				socket_data.is_can_write_ = false;

				del_channel_list_.emplace_back(socket_data.send_channel_);
				del_channel_list_.emplace_back(socket_data.recv_channel_);

				socket_ops::closeSocket(socket_data.sock_fd());

				if (socket_data.disconnect_callback_)
					socket_data.disconnect_callback_();
			}

			bool IocpImpl::asyncConnect(sock_t fd, const EndPoint& addr , Channel* op)
			{
				if (::CreateIoCompletionPort((HANDLE)fd, handle_, 0, 0) == 0)
				{
					return false;
				}

				if (!connect_ex_)
				{
					DWORD bytes = 0;
					GUID guid = { 0x25a207b9, 0xddf3, 0x4660,
									{ 0x8e, 0xe9, 0x76, 0xe5, 0x8c, 0x74, 0x06, 0x3e } };
					void* ptr = nullptr; 
					if (::WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
						&guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0)
					{
						// ...
					}
					connect_ex_ = ptr; 
				}

				if (connect_ex_)
				{
					union address_union
					{
						struct sockaddr head;
						struct sockaddr_in v4;
						struct sockaddr_in6 v6;
					} address;
					memset(&address, 0, sizeof(address));
					address.head.sa_family = addr.family();
					auto ret = ::bind(fd, &address.head, addr.family() == AF_INET ? sizeof(address.v4) : sizeof(address.v6));
					if (ret == SPDNET_SOCKET_ERROR) {
						// ...
						return false; 
					}
					void* connect_ex = connect_ex_; 
					typedef BOOL(PASCAL* connect_ex_fn)(SOCKET,
						const sockaddr*, int, void*, DWORD, DWORD*, OVERLAPPED*);
					BOOL result = ((connect_ex_fn)connect_ex)(fd,
						addr.socket_addr(), addr.socket_addr_len(), 0, 0, 0, (LPOVERLAPPED)op);
					DWORD last_error = ::WSAGetLastError();
					if (!result && last_error != WSA_IO_PENDING) {
						// ...
						return false; 
					}
				}
				return true;
			}

			void IocpImpl::send(SocketImplData& socket_data, const char* data, size_t len) {
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
					}
			    });
			}

            void IocpImpl::flushBuffer(SocketImplData& socket_data)
            {
				assert(loop_ref_.isInLoopThread());
				{
					std::lock_guard<SpinLock> lck(socket_data.send_guard_);
					if (SPDNET_PREDICT_TRUE(socket_data.pending_buffer_list_.empty())) {
						socket_data.pending_buffer_list_.swap(socket_data.send_buffer_list_);
					}
					else {
						for (const auto buffer : socket_data.send_buffer_list_)
							socket_data.pending_buffer_list_.push_back(buffer);
						socket_data.send_buffer_list_.clear();
					}
				}

				constexpr size_t MAX_BUF_CNT = 1024;
				WSABUF send_buf[MAX_BUF_CNT]; 

				size_t cnt = 0;
				size_t prepare_send_len = 0;
				for (const auto buffer : socket_data.pending_buffer_list_) {
					send_buf[cnt].buf = buffer->getDataPtr();
					send_buf[cnt].len = buffer->getLength();
					cnt++;
					if (cnt >= MAX_BUF_CNT)
						break;
				}
				assert(cnt > 0);

				DWORD send_len = 0;
				const int result = ::WSASend(socket_data.sock_fd(),
					send_buf,
					cnt,
					&send_len,
					0,
					(LPOVERLAPPED)socket_data.send_channel_.get(),
					0);
				DWORD last_error = current_errno(); 
				if (result != 0 && last_error != WSA_IO_PENDING) {
					closeSocket(socket_data);
				}
            }

            void IocpImpl::startRecv(SocketImplData& socket_data)
            {
                static WSABUF  buf = { 0, 0 };
                buf.len = socket_data.recv_buffer_.getWriteValidCount();
                buf.buf = socket_data.recv_buffer_.getWritePtr(); 

				DWORD bytes_transferred = 0;
				DWORD recv_flags = 0;
				socket_data.recv_channel_->reset(); 
                int result  = ::WSARecv(socket_data.sock_fd(), &buf, 1, &bytes_transferred, &recv_flags, (LPOVERLAPPED)socket_data.recv_channel_.get(), 0);
                DWORD last_error = ::WSAGetLastError();
                if (result != 0 && last_error != WSA_IO_PENDING) {
                    closeSocket(socket_data);
                }
            }

			void IocpImpl::wakeup()
			{
				::PostQueuedCompletionStatus(handle_,0,0, (LPOVERLAPPED)wakeup_op_.get());
			}

            void IocpImpl::runOnce(uint32_t timeout)
            {
				for (;;)
				{
					DWORD bytes_transferred = 0;
					ULONG_PTR completion_key = 0;
					LPOVERLAPPED overlapped = 0;
					::SetLastError(0);
					BOOL ok = ::GetQueuedCompletionStatus(handle_, &bytes_transferred, &completion_key, &overlapped, timeout);
					DWORD last_error = ::GetLastError();

					if (overlapped) {
						Channel* op = static_cast<Channel*>(overlapped);
						op->doComplete((size_t)bytes_transferred , std::error_code(last_error , std::system_category()));

					}
					else if (!ok) {
						break;
					}

					timeout = 0; 
				}

				del_channel_list_.clear();
            }

        }
    }

}

#endif