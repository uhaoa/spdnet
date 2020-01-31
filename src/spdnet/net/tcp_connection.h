#ifndef SPDNET_NET_TCP_CONNECTION_H_
#define SPDNET_NET_TCP_CONNECTION_H_
#include <memory>
#include <deque>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>
#include <spdnet/net/channel.h>
#include <spdnet/base/buffer.h>

namespace spdnet
{
namespace net
{
    class EventLoop ; 

    class TcpConnection : public Channel , public base::NonCopyable , public std::enable_shared_from_this<TcpConnection>
    {
     public:
        friend class EventLoop; 
        using Ptr = std::shared_ptr<TcpConnection> ; 
        using EventLoopPtr = std::shared_ptr<EventLoop> ; 
        using TcpDisconnectCallback = std::function<void(Ptr)> ;  
        using TcpDataCallback   = std::function<size_t(const char* , size_t len)> ;  
        using MessagePtr = std::shared_ptr<std::string> ; 

        TcpConnection(TcpSocket::Ptr socket , EventLoopPtr) ; 

        void TrySend() override ; 
        void TryRecv() override ;
        void OnClose() override ; 
        void PostShutDown();
        void PostDisconnect(); 
        void ExecShutDownInLoop();

        void SetDisconnectCallback(TcpDisconnectCallback&& callback);
        void SetDataCallback(TcpDataCallback&& callback);
        void SetMaxRecvBufferSize(size_t len){
           max_recv_buffer_size_ = len ; 
        }
        void GrowRecvBufferSize() ; 
        void AddWriteEvent();
        void RemoveWriteEvent();
        void Send(const char* data , size_t len);
     public:
        static Ptr Create(TcpSocket::Ptr socket , EventLoopPtr loop) ; 

     private:
        void ExecFlushAfterLoop();
        void FlushBufferBySend();
        void FlushBufferByWriteV();
        int GetSocketFd()const{
           return socket_->sock_fd();
        }

        class BufferDeleter
        {
           public:
               void operator()(struct buffer_s* buffer){
                  ox_buffer_delete(buffer) ; 
               }
        };

     private:
        TcpSocket::Ptr                    socket_ ; 
        EventLoopPtr                      loop_owner_ ; 
        TcpDisconnectCallback             disconnect_callback_ ; 
        TcpDataCallback                   data_callback_ ; 
        size_t                            max_recv_buffer_size_ = 64 * 1024; 
        std::unique_ptr<struct buffer_s , BufferDeleter>  recv_buffer_ ; 
        std::unique_ptr<struct buffer_s , BufferDeleter>  send_buffer_ ; 
        volatile bool                              closed = false;

        struct PendingMessage
        {
            MessagePtr  data_ ; 
            size_t     left_size_ ; 
        };

		std::deque<PendingMessage>  send_list_ ;
        bool						is_post_flush_; 
        bool						is_can_write_;
         
    } ; 

}
}


#endif // SPDNET_NET_TCP_CONNECTION_H_