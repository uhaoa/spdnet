#ifndef SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SEND_CHANNEL_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SEND_CHANNEL_H_
#include <system_error>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/detail/impl_win/iocp_channel.h>
#include <spdnet/base/buffer_pool.h>

namespace spdnet {
    namespace net {
        namespace detail {
			class IocpSendChannel : public SocketChannel
			{
			public:
				IocpSendChannel(SocketData& data, std::shared_ptr<IoObjectImplType> io_impl)
					:SocketChannel(data, io_impl)
				{

				}

				void doComplete(size_t bytes_transferred, std::error_code ec) override
				{
					if (bytes_transferred == 0 || ec) {
						io_impl_->closeSocket(socket_data_);
					}
					else {
						auto send_len = bytes_transferred;
						for (auto iter = socket_data_.pending_buffer_list_.begin();
							iter != socket_data_.pending_buffer_list_.end();) {
							auto buffer = *iter;
							if (SPDNET_PREDICT_TRUE(buffer->getLength() <= send_len)) {
								send_len -= buffer->getLength();
								buffer->clear();
								spdnet::base::BufferPool::instance().recycleBuffer(buffer);
								iter = socket_data_.pending_buffer_list_.erase(iter);
							}
							else {
								buffer->removeLength(send_len);
								break;
							}

						}

						socket_data_.is_post_flush_ = false;
					}
				}
			};
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_IOCP_SEND_CHANNEL_H_