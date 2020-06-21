#ifndef SPDNET_NET_DETAIL_IMPL_WIN_SOCKET_OP_H_
#define SPDNET_NET_DETAIL_IMPL_WIN_SOCKET_OP_H_

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <spdnet/net/event_loop.h>

namespace spdnet {
    namespace net {
        namespace detail {
            class Operation : public OVERLAPPED
            {
                virtual void doComplete(size_t bytes_transferred) = 0;
            };

            class SocketOp : public Operation {
            public:
                SocketOp(IocpImpl::SocketImplData& data, std::shared_ptr<EventLoop> loop)
                    :socket_data_(data), loop_(loop)
                {

                }
                virtual ~SocketOp() noexcept {}
            protected:
				IocpImpl::SocketImplData& socket_data_;
                std::shared_ptr<EventLoop> loop_;
            };

            class SocketWakeupOp : public Operation
            {
                void doComplete(size_t bytes_transferred) override
                {
                    // ...
                }
            };

            class SocketSendOp : public SocketOp 
            {
            public:
                SocketSendOp(IocpImpl::SocketImplData& data , std::shared_ptr<EventLoop> loop)
                    :SocketOp(data , loop)
                {

                }
                void doComplete(size_t bytes_transferred) override
                {
					if (bytes_transferred == 0) {
                        //closeSocket(socket_data);
					}
                    else {
                        auto send_len = bytes_transferred; 
						for (auto iter = socket_data_.pending_buffer_list_.begin();
							iter != socket_data_.pending_buffer_list_.end();) {
							auto buffer = *iter;
							if (SPDNET_PREDICT_TRUE(buffer->getLength() <= send_len)) {
                                send_len -= buffer->getLength();
								buffer->clear();
                                loop_->recycleBuffer(buffer);
								iter = socket_data_.pending_buffer_list_.erase(iter);
							}
							else {
								buffer->removeLength(send_len);
								break;
							}

						}
                    }
                }
            };

			class SocketRecieveOp : public SocketOp
			{
			public:
                SocketRecieveOp(IocpImpl::SocketImplData& data, std::shared_ptr<EventLoop> loop)
                    :SocketOp(data, loop)
                {

                }
				void doComplete(size_t bytes_transferred) override
				{
                    bool force_close = false; 
                    if (bytes_transferred == 0) {
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
                    
                    /*
                    if (force_close)
                        impl_.closeSocket(socket_data_);
                    */

                    loop_->getImpl().startRecv(socket_data_);
				}
			};
        }
    }

}

#endif  // SPDNET_NET_DETAIL_IMPL_WIN_SOCKET_OP_H_