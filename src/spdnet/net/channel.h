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
    private:
        virtual void trySend() = 0;
        virtual void tryRecv() = 0;
        virtual void onClose() = 0; 
        friend class EventLoop;
    };


    class WakeupChannel : public Channel
    {
     public:
        explicit WakeupChannel(int fd)noexcept ; 
        bool wakeup()
        {
            int data = 1 ; 
            return write(fd_ , &data , sizeof(data)) == 0;
        }
     private:
        void trySend() override 
        {

        }

        void tryRecv() override 
        {
            char buf[1024]{0} ; 
            while(true)
            {
                auto ret = read(fd_ , buf , sizeof(buf)); 
                if(ret == -1 || static_cast<size_t>(ret) < sizeof(buf))
                    break; 
            }
        }

        void onClose() override 
        {

        }
     private:
		 int fd_ ;
    } ;
}
}

#endif  // SPDNET_NET_CHANNEL_H_