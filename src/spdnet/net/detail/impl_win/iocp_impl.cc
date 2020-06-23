#include <spdnet/base/platform.h>
#ifdef SPDNET_PLATFORM_WINDOWS
#include <memory>
#include <iostream>
#include <cassert>
#include <spdnet/net/exception.h>
#include <spdnet/net/tcp_session.h>
#include <spdnet/net/event_loop.h>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_operation.h>

namespace spdnet {
    namespace net {
		namespace detail {
			SocketImplData::SocketImplData(std::shared_ptr<EventLoop> loop, std::shared_ptr<TcpSocket> socket)
				:socket_(std::move(socket))
			{
				recv_op_ = std::make_shared<SocketRecieveOp>(*this, loop);
				send_op_ = std::make_shared<SocketSendOp>(*this, loop);
			}

			IocpImpl::IocpImpl(EventLoop& loop) noexcept
				: handle_(CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1)), loop_ref_(loop)
			{
				wakeup_op_ = std::make_shared<SocketWakeupOp>();
			}

			IocpImpl::~IocpImpl() noexcept {
				CloseHandle(handle_);
				handle_ = INVALID_HANDLE_VALUE;
			}

			bool IocpImpl::onSocketEnter(SocketImplData& socket_data) {
				if (CreateIoCompletionPort((HANDLE)socket_data.sock_fd(), handle_, 0, 0) == 0)
					return false;
				startRecv(socket_data);
				return true;
			}

			void IocpImpl::startAccept(sock listen_fd)
			{
				/*
				assert(op);
				DWORD bytes_read = 0;
				BOOL result = ::AcceptEx(v, op->makeNewSocket(), op->buffer() ,
					0, op->getAddressLength(), op->getAddressLength(), &bytes_read, op);
				DWORD last_error = current_errno();
				if (!result && last_error != WSA_IO_PENDING) {
					// error
					std::cout << "::AcceptEx error ! errno:" << last_error << std::endl;. 
				}
				*/
			}

			void IocpImpl::send(SocketImplData& socket_data, const char* data, size_t len) {
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
					socket_data.send_op_.get(),
					0);

				if (result != 0 && last_error != WSA_IO_PENDING) {
					// closeSocket(socket_data);
				}
            }

            void IocpImpl::startRecv(SocketImplData& socket_data)
            {
                static WSABUF  buf = { 0, nullptr };
                buf.len = socket_data.recv_buffer_.getWriteValidCount();
                buf.buf = socket_data.recv_buffer_.getWritePtr(); 

				DWORD bytes_transferred = 0;
				DWORD recv_flags = 0;
                int result  = ::WSARecv(socket_data.sock_fd(), &buf, 1, &bytes_transferred, &recv_flags, socket_data.recv_op_.get(), 0);
                DWORD last_error = ::WSAGetLastError();
                if (result != 0 && last_error != WSA_IO_PENDING) {
                    // closeSocket(socket_data);
                }
            }

			void IocpImpl::wakeup()
			{
				PostQueuedCompletionStatus(handle_,0,0,wakeup_op_.get());
			}

            void IocpImpl::runOnce(uint32_t timeout)
            {
				for (;;)
				{
					DWORD bytes_transferred = 0;
					dword_ptr_t completion_key = 0;
					LPOVERLAPPED overlapped = 0;
					::SetLastError(0);
					/*BOOL ok = */::GetQueuedCompletionStatus(handle_, &bytes_transferred, &completion_key, &overlapped, timeout);
					DWORD last_error = ::GetLastError();

					if (overlapped) {
						Operation* op = static_cast<Operation*>(overlapped);
						op->doComplete(bytes_transferred);

					}
					else/* if (!ok) */{
						break;
					}

					timeout = 0; 
				}
            }

        }
    }

}

#endif