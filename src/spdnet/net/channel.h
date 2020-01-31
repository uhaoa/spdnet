#ifndef SPDNET_NET_CHANNEL_H_
#define SPDNET_NET_CHANNEL_H_
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/base.h>

namespace spdnet
{
namespace net
{
    class EventLoop ; 
    class Channel
    {
    public:
        virtual ~Channel()noexcept {}
    /* 
        explicit Channel(int fd , EventLoop* loop)noexcept ; 
        virtual ~Channel()noexcept ; 
    */
    private:
        virtual void TrySend() = 0;
        virtual void TryRecv() = 0;
        virtual void OnClose() = 0; 

        friend class EventLoop;
    };


    class WakeupChannel : public Channel
    {
     public:
        explicit WakeupChannel(int fd)noexcept ; 
        bool Wakeup()
        {
            int data = 1 ; 
            return write(fd_ , &data , sizeof(data)) == 0;
        }
     private:
        void TrySend() override 
        {

        }

        void TryRecv() override 
        {
            char buf[1024]{0} ; 
            while(true)
            {
                auto ret = read(fd_ , buf , sizeof(buf)); 
                if(ret == -1 || static_cast<size_t>(ret) < sizeof(buf))
                    break; 
            }
        }

        void OnClose() override 
        {

        }
     private:
		 int fd_ ;
    } ;
}
}

#endif  // SPDNET_NET_CHANNEL_H_