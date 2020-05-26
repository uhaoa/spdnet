#include <spdnet/net/tcp_connection.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/event_loop.h>

namespace spdnet
{
namespace net
{
    TcpConnection::TcpConnection(std::shared_ptr<TcpSocket> socket , EventLoopPtr loop)
        : socket_(std::move(socket)) , 
          loop_owner_(loop) , 
          is_post_flush_(false), 
          is_can_write_(true)
    {

    }

    TcpConnection::Ptr TcpConnection::create(std::shared_ptr<TcpSocket> socket , EventLoopPtr loop)
    {
        return std::make_shared<TcpConnection>(std::move(socket) , loop); 
    }

    void TcpConnection::regWriteEvent()
    {
        struct epoll_event event { 0  , {nullptr}} ;
        event.events = EPOLLET | EPOLLIN | EPOLLOUT| EPOLLRDHUP;
        event.data.ptr = (Channel*)(this) ; 
        ::epoll_ctl(loop_owner_->epoll_fd() , EPOLL_CTL_MOD , socket_->sock_fd() , &event)  ; 
    }

    void TcpConnection::unregWriteEvent()
    {
        struct epoll_event event{ 0 , {nullptr}} ;
        event.events = EPOLLET | EPOLLIN | EPOLLRDHUP ; 
        event.data.ptr = (Channel*)(this) ;
        ::epoll_ctl(loop_owner_->epoll_fd() , EPOLL_CTL_MOD , socket_->sock_fd() , &event); 
    }

    void TcpConnection::send(const char* data , size_t len)
    {
        if(len <= 0)
            return ; 
		{
			std::lock_guard<std::mutex> lck(send_guard_);
			send_buffer_.write(data, len);
		}
		if (is_post_flush_) {
			return;
		}
		is_post_flush_ = true; 
        auto this_ptr = shared_from_this();
        loop_owner_->runInEventLoop([this_ptr]() {
			if (this_ptr->is_can_write_) {
				this_ptr->flushBuffer();
				this_ptr->is_post_flush_ = false; 
			}
        });
    }

    void TcpConnection::flushBuffer()
    {
		bool force_close = false;
		assert(loop_owner_->isInLoopThread());
		{
			std::lock_guard<std::mutex> lck(send_guard_);
			if (pending_buffer_.getLength() == 0) {
				pending_buffer_.swap(send_buffer_);
			}
			else {
				pending_buffer_.write(send_buffer_.getDataPtr(), send_buffer_.getLength());
				send_buffer_.clear();
			}
		}

		if (pending_buffer_.getLength() == 0)
			return;

		size_t send_len = ::send(socket_->sock_fd() , pending_buffer_.getDataPtr() ,static_cast<int>(pending_buffer_.getLength()) , 0);
		if (send_len < 0) {
			if (errno == EAGAIN) {
				regWriteEvent();
				is_can_write_ = false;
			}
			else {
				force_close = true;
			}
		}
		else
		{
			pending_buffer_.removeLength(send_len);
			if (pending_buffer_.getLength() == 0)
				pending_buffer_.adjustToHead();
		}
		if (force_close) 
			onClose();
    }

    void TcpConnection::trySend() 
    {
        is_can_write_ = true; 
        unregWriteEvent()  ; 
		flushBuffer();
    }

    void TcpConnection::tryRecv()
    {
		char stack_buffer[65536]; 
        bool force_close = false;
        while (true)
        {
			size_t valid_count = recv_buffer_.getWriteValidCount(); 
			struct iovec vec[2];
			vec[0].iov_base = stack_buffer;
			vec[0].iov_len = sizeof(stack_buffer);
			vec[1].iov_base = recv_buffer_.getWritePtr();
			vec[1].iov_len = valid_count;


			size_t try_recv_len = valid_count + sizeof(stack_buffer);

            int recv_len = static_cast<int>(::readv(socket_->sock_fd() , vec, 2));
            if(recv_len == 0 || (recv_len < 0 && errno != EAGAIN)) {
                force_close = true;
                break;
            }
			size_t stack_len = 0;
			if (recv_len > (int)valid_count) {
				recv_buffer_.addWritePos(valid_count);
				stack_len = recv_len - valid_count;
			}
			else
				recv_buffer_.addWritePos(recv_len);

            if(nullptr != data_callback_) {
                size_t len = data_callback_(recv_buffer_.getDataPtr() , recv_buffer_.getLength());
                assert(len <= recv_buffer_.getLength()) ;
                if(len == recv_buffer_.getLength()){
					recv_buffer_.removeLength(len);
					if (stack_len > 0) {
						len = data_callback_(stack_buffer, stack_len);
						assert(len <= stack_len);
						if (len < stack_len) {
							recv_buffer_.write(stack_buffer + len , stack_len  - len);
						}
						else if(len > stack_len){
							force_close = true; 
							break;
						}
					}
                }
				else if (len < recv_buffer_.getLength()) {
					recv_buffer_.removeLength(len);
					if (stack_len > 0)
						recv_buffer_.write(stack_buffer, stack_len);
				}
				else {
					force_close = true;
					break;
				}

            }

			if (recv_len >= (int)recv_buffer_.getCapacity()/* + (int)(sizeof(stack_buffer))*/) {
				size_t grow_len = 0; 
				if (recv_buffer_.getCapacity() * 2 <= max_recv_buffer_size_)
					grow_len = recv_buffer_.getCapacity();
				else
					grow_len = max_recv_buffer_size_ - recv_buffer_.getCapacity();

				if (grow_len > 0)
					recv_buffer_.grow(grow_len);
			}

			if (recv_buffer_.getWriteValidCount() == 0 || recv_buffer_.getLength() == 0) 
				recv_buffer_.adjustToHead();

            if(recv_len < static_cast<int>(try_recv_len))
                break;
        }
        
        if(force_close)
            onClose();
    }

    void TcpConnection::onClose()
    {
        if(has_closed)
            return  ; 
		assert(loop_owner_->isInLoopThread());
		has_closed = true;
        auto loop       = loop_owner_;
        auto callback   = disconnect_callback_ ; 
        Ptr connection  = shared_from_this(); 
        std::shared_ptr<TcpSocket> socket  = std::move(socket_); 
        loop_owner_->runAfterEventLoop([loop , callback , connection , socket](){
            if(callback)
                callback(connection);
            loop->removeTcpConnection(socket->sock_fd());
            struct epoll_event ev {0 , {nullptr}};
            ::epoll_ctl(loop->epoll_fd() , EPOLL_CTL_DEL , socket->sock_fd() , &ev) ; 
        }) ;
        is_can_write_ = false;
        disconnect_callback_ = nullptr ; 
        data_callback_ = nullptr ; 
        socket_.reset();
    }

    void TcpConnection::postDisconnect()
    {
        auto loop = loop_owner_ ; 
        auto this_ptr = shared_from_this();
        loop_owner_->runInEventLoop([loop , this_ptr](){
			this_ptr->onClose(); 
        }) ; 
    }

    void TcpConnection::postShutDown()
    {
        auto loop = loop_owner_ ; 
        auto this_ptr = shared_from_this();
        loop_owner_->runInEventLoop([loop , this_ptr](){
            loop->runAfterEventLoop([this_ptr](){
				this_ptr->execShutDownInLoop();
            }) ; 
        }) ; 
    }

    void TcpConnection::execShutDownInLoop()
    {
        assert(socket_ != nullptr);
		assert(loop_owner_->isInLoopThread());
		if (socket_ != nullptr) {
			::shutdown(socket_->sock_fd(), SHUT_WR);
		}
        is_can_write_ = false;
    }

    void TcpConnection::setDisconnectCallback(TcpDisconnectCallback&& callback)
    {
        disconnect_callback_ = std::move(callback);
    }

    void TcpConnection::setDataCallback(TcpDataCallback&& callback)
    {
        data_callback_ = std::move(callback);
    }

}
}