#ifndef SPDNET_NET_TCP_SESSION_H_
#define SPDNET_NET_TCP_SESSION_H_
#include <memory>
#include <deque>
#include <mutex>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/channel.h>
#include <spdnet/base/buffer.h>

namespace spdnet
{
namespace net
{
    class EventLoop ; 

    class TcpSession : public Channel , public base::NonCopyable , public std::enable_shared_from_this<TcpSession>
    {
     public:
        friend class EventLoop; 
        using Ptr = std::shared_ptr<TcpSession> ; 
        using EventLoopPtr = std::shared_ptr<EventLoop> ; 
        using TcpDisconnectCallback = std::function<void(Ptr)> ;  
        using TcpDataCallback   = std::function<size_t(const char* , size_t len)> ;
		using TcpEnterCallback = std::function<void(TcpSession::Ptr)>;

        TcpSession(std::shared_ptr<TcpSocket> socket , EventLoopPtr) ; 

        void trySend() override ; 
        void tryRecv() override ;
        void onClose() override ; 

        void postShutDown();
        void postDisconnect(); 
        void execShutDownInLoop();

        void setDisconnectCallback(TcpDisconnectCallback&& callback);
        void setDataCallback(TcpDataCallback&& callback);
        void setMaxRecvBufferSize(size_t len){
           max_recv_buffer_size_ = len ; 
        }
        void setNodelay() {
            socket_->setNoDelay();
        }
        void regWriteEvent();
        void unregWriteEvent();
        void send(const char* data , size_t len);
     public:
        static Ptr create(std::shared_ptr<TcpSocket> socket , EventLoopPtr loop) ;

     private:
        void flushBuffer();
        int getSocketFd()const{
           return socket_->sock_fd();
        }

     private:
        std::shared_ptr<TcpSocket>        socket_ ; 
        EventLoopPtr                      loop_owner_ ; 
        TcpDisconnectCallback             disconnect_callback_ ; 
        TcpDataCallback                   data_callback_ ; 
        size_t                            max_recv_buffer_size_ = 64 * 1024; 
        Buffer							  recv_buffer_ ; 
        Buffer							  send_buffer_ ;
        Buffer							  pending_buffer_;
		std::mutex                        send_guard_;
        volatile bool                     has_closed = false;

		volatile bool						is_post_flush_;
		volatile bool						is_can_write_;
         
    } ; 

}
}


#endif // SPDNET_NET_TCP_SESSION_H_