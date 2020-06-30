#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP_RECV_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP_RECV_CHANNEL_H_
#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>

namespace spdnet {
    namespace net {
        namespace detail {
			class IocpRecieveChannel : public SocketChannel
			{
			public:
				IocpRecieveChannel(TcpSocketData& data, std::shared_ptr<IoObjectImplType> impl)
                    :SocketChannel(data, impl)
                {

                }
				void startRecv()
				{
					static WSABUF  buf = { 0, 0 };
					buf.len = socket_data_.recv_buffer_.getWriteValidCount();
					buf.buf = socket_data_.recv_buffer_.getWritePtr();

					DWORD bytes_transferred = 0;
					DWORD recv_flags = 0;
					socket_data_.recv_channel_->reset();
					int result = ::WSARecv(socket_data_.sock_fd(), &buf, 1, &bytes_transferred, &recv_flags, (LPOVERLAPPED)this, 0);
					DWORD last_error = ::WSAGetLastError();
					if (result != 0 && last_error != WSA_IO_PENDING) {
						closeSocket(socket_data_);
					}
				}
			private:
				void doComplete(size_t bytes_transferred, std::error_code ec) override
				{
                    bool force_close = false; 
                    if (bytes_transferred == 0 || ec) {
                        // eof 
                        force_close = true; 
                    }
                    else {
                        auto& recv_buffer = socket_data_.recv_buffer_; 
                        auto post_len = recv_buffer.getWriteValidCount(); 
                        recv_buffer.addWritePos(bytes_transferred);
                        if (nullptr != socket_data_.data_callback_) {
							size_t len = socket_data_.data_callback_(recv_buffer.getDataPtr(), recv_buffer.getLength());
							assert(len <= recv_buffer.getLength());
							if (len <= recv_buffer.getLength()) {
                                recv_buffer.removeLength(len); 
                            }
                            else {
                                force_close = true; 
                            }
                        }

                        if (post_len == bytes_transferred) {
							size_t grow_len = 0;
							if (recv_buffer.getCapacity() * 2 <= socket_data_.max_recv_buffer_size_)
								grow_len = recv_buffer.getCapacity();
							else
								grow_len = socket_data_.max_recv_buffer_size_ - recv_buffer.getCapacity();

							if (grow_len > 0)
								recv_buffer.grow(grow_len);
                        }

						if (SPDNET_PREDICT_FALSE(recv_buffer.getWriteValidCount() == 0 || recv_buffer.getLength() == 0))
							recv_buffer.adjustToHead();
                    }
                    
                    
                    if (force_close)
                        io_impl_->closeSocket(socket_data_);
                    else 
						io_impl_->startRecv(socket_data_);
				}
			};
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP_RECV_CHANNEL_H_