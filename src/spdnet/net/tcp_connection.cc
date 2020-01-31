#include <spdnet/net/tcp_connection.h>
#include <cassert>
#include <iostream>
#include <string.h>
#include <spdnet/net/event_loop.h>

namespace spdnet
{
namespace net
{
    TcpConnection::TcpConnection(TcpSocket::Ptr socket , EventLoopPtr loop) 
        : socket_(std::move(socket)) , 
          loop_owner_(loop) , 
          is_post_flush_(false), 
          is_can_write_(true)
    {
        recv_buffer_.reset(ox_buffer_new(std::min<size_t>(10 * 1024 , max_recv_buffer_size_))) ; 
        send_buffer_.reset(ox_buffer_new(64 * 1024)) ; 
    }

    TcpConnection::Ptr TcpConnection::Create(TcpSocket::Ptr socket , EventLoopPtr loop) 
    {
        return std::make_shared<TcpConnection>(std::move(socket) , loop); 
    }

    void TcpConnection::AddWriteEvent()
    {
        struct epoll_event event { 0  , {nullptr}} ;
        event.events = EPOLLET | EPOLLIN | EPOLLOUT| EPOLLRDHUP;
        event.data.ptr = (Channel*)(this) ; 
        ::epoll_ctl(loop_owner_->epoll_fd() , EPOLL_CTL_MOD , socket_->sock_fd() , &event)  ; 
    }

    void TcpConnection::RemoveWriteEvent()
    {
        struct epoll_event event{ 0 , {nullptr}} ;
        event.events = EPOLLET | EPOLLIN | EPOLLRDHUP ; 
        event.data.ptr = (Channel*)(this) ;
        ::epoll_ctl(loop_owner_->epoll_fd() , EPOLL_CTL_MOD , socket_->sock_fd() , &event); 
    }

    void TcpConnection::Send(const char* data , size_t len)
    {
        if(len <= 0)
            return ; 
		MessagePtr msg = std::make_shared<std::string>(data , len);
        auto shared_this = shared_from_this();
        loop_owner_->RunInEventLoop([msg, shared_this](){
            size_t msg_len = msg->size();
            shared_this->send_list_.emplace_back(PendingMessage{std::move(msg) , msg_len});
            shared_this->ExecFlushAfterLoop();
        });
    }

    void TcpConnection::ExecFlushAfterLoop()
    {
        if(!send_list_.empty() && !is_post_flush_ && is_can_write_) 
        {
            auto shared_this = shared_from_this();
            loop_owner_->RunAfterEventLoop([shared_this](){
                shared_this->is_post_flush_ = false;
                shared_this->FlushBufferByWriteV();
            });

            is_post_flush_ = true;
        }
    }

    void TcpConnection::FlushBufferByWriteV()
    {
        bool must_close = false;
        if(!is_can_write_ || send_list_.empty())
            return ; 

		//if (send_list_.size() == 1) {
		//	FlushBufferBySend();
		//}

        constexpr int MAX_IO_BUF_CNT = 1024 ; 
        struct iovec io_buff[MAX_IO_BUF_CNT] ;
        int cnt = 0 ; 
        size_t ready_send_len = 0 ; 
        for(auto& send_buf : send_list_ ) {
            assert(send_buf.left_size_ > 0);
            io_buff[cnt].iov_base = static_cast<void*>(const_cast<char*>(send_buf.data_->c_str() + (send_buf.data_->size() - send_buf.left_size_))); 
            io_buff[cnt].iov_len = send_buf.left_size_ ; 
            ready_send_len += send_buf.left_size_ ; 
            if(++cnt >= MAX_IO_BUF_CNT)
                break;
        }

        size_t send_len = static_cast<size_t>(writev(socket_->sock_fd() , (struct iovec*)&io_buff , cnt)) ; 
        if(send_len < 0) {
            if(errno == EAGAIN) {
                AddWriteEvent();
                is_can_write_ = false;
            }
            else
                must_close = true;
        }
        else
        {
            size_t len = send_len ; 
            auto iter = send_list_.begin();
            while(iter != send_list_.end()) {
                if(iter->left_size_ <=  len) {
                    len -= iter->left_size_ ; 
                    iter = send_list_.erase(iter) ; 
                }
                else {
                    iter->left_size_ -= len ; 
                    break;
                }
            }
        }
        
        if(must_close)
            OnClose();
    }

    void TcpConnection::FlushBufferBySend()
    {
        bool must_close = false;
        while (!send_list_.empty() && is_can_write_) 
		{
            size_t buf_len = ox_buffer_getwritevalidcount(send_buffer_.get()) ; 
            size_t buf_left_len = buf_len ; 
            auto iter_beg = send_list_.begin() ; 
            while (iter_beg != send_list_.end())
			{
                if(buf_left_len == 0)
                    break;
                if(iter_beg->left_size_ <= buf_left_len) {
                    memcpy(ox_buffer_getwriteptr(send_buffer_.get()) , iter_beg->data_->c_str() + (iter_beg->data_->size() - iter_beg->left_size_) , iter_beg->left_size_);
                    buf_left_len -= iter_beg->left_size_ ;  
                    iter_beg = send_list_.erase(iter_beg);
                }
                else {
                    memcpy(ox_buffer_getwriteptr(send_buffer_.get()) , iter_beg->data_->c_str() + (iter_beg->data_->size() - iter_beg->left_size_) , buf_left_len);
                    iter_beg->left_size_ -= buf_left_len ; 
                    buf_left_len = 0 ; 
                    break;
                }
            }

            ox_buffer_addwritepos(send_buffer_.get() , buf_len - buf_left_len); 
            size_t prepare_send_size = ox_buffer_getreadvalidcount(send_buffer_.get());
            if(prepare_send_size == 0)
                break;

            size_t send_size =  static_cast<size_t>(::send(socket_->sock_fd() , ox_buffer_getreadptr(send_buffer_.get()) , prepare_send_size , 0)) ; 

            if(send_size <= 0) {
                if(errno == EAGAIN) {
                    AddWriteEvent();
                    is_can_write_ = false;
                }
                else
                    must_close = true; 
                break;
            }
            else
            {
                ox_buffer_addreadpos(send_buffer_.get()  , static_cast<size_t>(send_size));
                if(send_size < prepare_send_size) {
                    AddWriteEvent();
                    is_can_write_ = false;
                    break;
                }
            }

            if(ox_buffer_getreadvalidcount(send_buffer_.get()) == 0 || ox_buffer_getwritevalidcount(send_buffer_.get()) == 0)
                ox_buffer_adjustto_head(send_buffer_.get()) ; 
   
        }

        if(must_close)
            OnClose();
    }

    void TcpConnection::TrySend() 
    {
        is_can_write_ = true; 
        RemoveWriteEvent()  ; 
      //  FlushSendBuffer() ;
        FlushBufferByWriteV();
    }

    void TcpConnection::TryRecv()
    {
        bool must_close = false;
        while (true)
        {
            size_t try_recv_len = ox_buffer_getwritevalidcount(recv_buffer_.get()) ; 
            if(try_recv_len <= 0)
                return ; 
            
            int recv_len = static_cast<int>(::recv(socket_->sock_fd() , ox_buffer_getwriteptr(recv_buffer_.get()) , try_recv_len , 0));
            if(recv_len == 0 || (recv_len < 0 && errno != EAGAIN))
            {
                must_close = true;
                break;
            }
            ox_buffer_addwritepos(recv_buffer_.get() , recv_len) ; 
            if(ox_buffer_getreadvalidcount(recv_buffer_.get()) == ox_buffer_getsize(recv_buffer_.get()))
            {
                GrowRecvBufferSize();
            }
            if(nullptr != data_callback_)
            {
                size_t len = data_callback_(ox_buffer_getreadptr(recv_buffer_.get()) , ox_buffer_getreadvalidcount(recv_buffer_.get()));
                assert(len <= ox_buffer_getreadvalidcount(recv_buffer_.get())) ;
                if(len <= ox_buffer_getreadvalidcount(recv_buffer_.get()))
                {
                    ox_buffer_addreadpos(recv_buffer_.get() , len) ; 
                }
                else{
                    break;
                }

            }
            if(ox_buffer_getwritevalidcount(recv_buffer_.get()) == 0 || ox_buffer_getreadvalidcount(recv_buffer_.get()) == 0)
            {
                ox_buffer_adjustto_head(recv_buffer_.get());
            }
            if(recv_len < static_cast<int>(try_recv_len))
            {
                break;
            }

        }
        
        if(must_close)
        {
            OnClose();
        }
    }

    void TcpConnection::OnClose()
    {
        if(closed)
            return  ; 

        closed = true;
        auto loop       = loop_owner_;
        auto callback   = disconnect_callback_ ; 
        Ptr connection  = shared_from_this(); 
        std::shared_ptr<TcpSocket> socket  = std::move(socket_); 
        loop_owner_->RunAfterEventLoop([loop , callback , connection , socket](){
            if(callback)
            {
                callback(connection);
            }
            loop->RemoveTcpConnection(socket->sock_fd());
            struct epoll_event ev {0 , {nullptr}};
            ::epoll_ctl(loop->epoll_fd() , EPOLL_CTL_DEL , socket->sock_fd() , &ev) ; 
        }) ;
        is_can_write_ = false;
        disconnect_callback_ = nullptr ; 
        data_callback_ = nullptr ; 
        recv_buffer_ = nullptr ; 
        socket_.reset();
    }

    void TcpConnection::PostDisconnect()
    {
        auto loop = loop_owner_ ; 
        auto this_share_ptr = shared_from_this();
        loop_owner_->RunInEventLoop([loop , this_share_ptr](){
            loop->RunAfterEventLoop([this_share_ptr](){
                this_share_ptr->OnClose();
            }) ; 
        }) ; 
    }

    void TcpConnection::PostShutDown()
    {
        auto loop = loop_owner_ ; 
        auto this_share_ptr = shared_from_this();
        loop_owner_->RunInEventLoop([loop , this_share_ptr](){
            loop->RunAfterEventLoop([this_share_ptr](){
                this_share_ptr->ExecShutDownInLoop();
            }) ; 
        }) ; 
    }

    void TcpConnection::ExecShutDownInLoop()
    {
        assert(socket_ != nullptr);
        assert(loop_owner_->IsInLoopThread());
        if(socket_ != nullptr)
        {
            ::shutdown(socket_->sock_fd() , SHUT_WR);
        }

        is_can_write_ = false;
    }

    void TcpConnection::SetDisconnectCallback(TcpDisconnectCallback&& callback)
    {
        disconnect_callback_ = std::move(callback);
    }

    void TcpConnection::SetDataCallback(TcpDataCallback&& callback)
    {
        data_callback_ = std::move(callback);
    }

    const static size_t GROW_BUFFER_SIZE = 1024 ; 

    void TcpConnection::GrowRecvBufferSize() 
    {
        assert(nullptr != recv_buffer_);
        size_t newlen = std::min<size_t>(ox_buffer_getsize(recv_buffer_.get()) + GROW_BUFFER_SIZE , max_recv_buffer_size_)  ; 
        std::unique_ptr<struct buffer_s , BufferDeleter> new_buffer(ox_buffer_new(newlen));
        ox_buffer_write(new_buffer.get() , ox_buffer_getreadptr(recv_buffer_.get()) , ox_buffer_getreadvalidcount(recv_buffer_.get())) ;
        recv_buffer_ = std::move(new_buffer);
    }

}
}