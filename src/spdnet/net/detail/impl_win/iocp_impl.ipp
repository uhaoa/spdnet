#ifndef SPDNET_NET_IOCP_IMPL_IPP_
#define SPDNET_NET_IOCP_IMPL_IPP_

#include <spdnet/net/detail/impl_win/iocp_impl.h>
#include <memory>
#include <iostream>
#include <cassert>
#include <spdnet/base/spin_lock.h>
#include <spdnet/base/buffer_pool.h>
#include <spdnet/net/detail/impl_win/iocp_wakeup_channel.h>
#include <spdnet/net/detail/impl_win/iocp_recv_channel.h>
#include <spdnet/net/detail/impl_win/iocp_send_channel.h>
#include <spdnet/net/task_executor.h>

namespace spdnet {
    namespace net {
        namespace detail {
            IocpImpl::IocpImpl(std::shared_ptr<TaskExecutor> task_executor,
                               std::function<void(sock_t)> &&socket_close_notify_cb) noexcept
                    : handle_(CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1)), wakeup_op_(handle_),
                      task_executor_(task_executor), socket_close_notify_cb_(socket_close_notify_cb) {
            }

            IocpImpl::~IocpImpl() noexcept {
                CloseHandle(handle_);
                handle_ = INVALID_HANDLE_VALUE;
            }

            bool IocpImpl::onSocketEnter(SocketData::Ptr data) {
                if (data->isServerSide()) {
                    if (CreateIoCompletionPort((HANDLE) data->sock_fd(), handle_, 0, 0) == 0) {
                        return false;
                    }
                }
                auto impl = shared_from_this();
                data->recv_channel_ = std::make_shared<IocpRecvChannel>(data, impl);
                data->send_channel_ = std::make_shared<IocpSendChannel>(data, impl);
                data->recv_channel_->startRecv();
                return true;
            }

            bool IocpImpl::startAccept(sock_t listen_fd, Channel *op) {
                if (::CreateIoCompletionPort((HANDLE) listen_fd, handle_, 0, 0) == 0) {
                    return false;
                }

                return true;
            }

            void IocpImpl::wakeup() {
                wakeup_op_.wakeup();
            }

            void IocpImpl::closeSocket(SocketData::Ptr data) {
                if (data->has_closed_)
                    return;

                del_channel_list_.emplace_back(data->send_channel_);
                del_channel_list_.emplace_back(data->recv_channel_);

                socket_close_notify_cb_(data->sock_fd());

                data->close();
            }

            bool IocpImpl::asyncConnect(sock_t fd, const EndPoint &addr, Channel *op) {
                if (::CreateIoCompletionPort((HANDLE) fd, handle_, 0, 0) == 0) {
                    return false;
                }

                if (!connect_ex_) {
                    DWORD bytes = 0;
                    GUID guid = {0x25a207b9, 0xddf3, 0x4660,
                                 {0x8e, 0xe9, 0x76, 0xe5, 0x8c, 0x74, 0x06, 0x3e}};
                    void *ptr = nullptr;
                    if (::WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                                   &guid, sizeof(guid), &ptr, sizeof(ptr), &bytes, 0, 0) != 0) {
                        // ...
                    }
                    connect_ex_ = ptr;
                }

                if (connect_ex_) {
                    union address_union {
                        struct sockaddr head;
                        struct sockaddr_in v4;
                        struct sockaddr_in6 v6;
                    } address;
                    memset(&address, 0, sizeof(address));
                    address.head.sa_family = addr.family();
                    auto ret = ::bind(fd, &address.head,
                                      addr.family() == AF_INET ? sizeof(address.v4) : sizeof(address.v6));
                    if (ret == SPDNET_SOCKET_ERROR) {
                        // ...
                        return false;
                    }
                    void *connect_ex = connect_ex_;
                    typedef BOOL(PASCAL
                    *connect_ex_fn)(SOCKET,
                    const sockaddr*, int, void *, DWORD, DWORD *, OVERLAPPED *);
                    BOOL result = ((connect_ex_fn) connect_ex)(fd,
                                                               addr.socket_addr(), addr.socket_addr_len(), 0, 0, 0,
                                                               (LPOVERLAPPED) op);
                    DWORD last_error = ::WSAGetLastError();
                    if (!result && last_error != WSA_IO_PENDING) {
                        // ...
                        return false;
                    }
                }
                return true;
            }

            void IocpImpl::send(SocketData::Ptr socket_data, const char *data, size_t len) {
                auto buffer =  allocBufferBySize(len);
                assert(buffer);
                buffer->write(data, len);
                {
                    std::lock_guard<spdnet::base::SpinLock> lck(socket_data->send_guard_);
                    socket_data->send_buffer_list_.push_back(buffer);
                }
                if (socket_data->is_post_flush_) {
                    return;
                }
                socket_data->is_post_flush_ = true;
                task_executor_->post([socket_data, this]() {
                    if (socket_data->is_can_write_) {
                        socket_data->send_channel_->flushBuffer();
                    }
                });
            }

            void IocpImpl::shutdownSocket(SocketData::Ptr data) {
                if (data->has_closed_)
                    return;
                ::shutdown(data->sock_fd(), SD_SEND);
                data->is_can_write_ = false;
            }

            /*
            void IocpImpl::wakeup()
            {
                ::PostQueuedCompletionStatus(handle_,0,0, (LPOVERLAPPED)wakeup_op_.get());
            }
            */
            void IocpImpl::runOnce(uint32_t timeout) {
                for (;;) {
                    DWORD bytes_transferred = 0;
                    ULONG_PTR completion_key = 0;
                    LPOVERLAPPED overlapped = 0;
                    ::SetLastError(0);
                    BOOL ok = ::GetQueuedCompletionStatus(handle_, &bytes_transferred, &completion_key, &overlapped,
                                                          timeout);
                    DWORD last_error = ::GetLastError();

                    if (overlapped) {
                        Channel *op = static_cast<Channel *>(overlapped);
                        op->doComplete((size_t) bytes_transferred, std::error_code(last_error, std::system_category()));

                    } else if (!ok) {
                        break;
                    }

                    timeout = 0;
                }

                del_channel_list_.clear();
            }

        }
    }

}

#endif // SPDNET_NET_IOCP_IMPL_IPP_
