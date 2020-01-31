#include <spdnet/base/socket_api.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

namespace spdnet 
{
namespace base
{
        bool InitSocketEnv() 
        {
            bool ret = true;       
            signal(SIGPIPE, SIG_IGN);
            return true; 
        }

        int SetSocketNoDelay(int fd)
        {
            int on = 1 ; 
            return setsockopt(fd , IPPROTO_TCP , TCP_NODELAY , (const char*)&on , sizeof(on)) ; 
        }

        bool SetSocketBlock(int fd)
        {
            unsigned long  mode = 0 ;     
            return ioctl(fd, FIONBIO, &mode) != -1 ; 
        }

        bool SetSocketNonBlock(int fd)
        {
            unsigned long  mode = 1 ;     
            return ioctl(fd, FIONBIO, &mode) != -1 ; 
        }

        int SetSocketSendBufSize(int fd , int size)
        {
            return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
        }

        int SetSocketRecvBufSize(int fd , int size)
        {
            return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
        }

        void CloseSocket(int fd)
        {       
            close(fd) ;    
        } 

        std::string GetIPFromSockaddr(const struct sockaddr* from) 
        {
            char tmp[64]{0} ; 
            sockaddr_in* pSin = (sockaddr_in*)&from;
            inet_ntop(AF_INET , (const void*)&pSin->sin_addr , tmp ,  sizeof(tmp) ) ;
            return tmp ;  
        }     

        std::string GetIPFromSockFd(int fd) {
            struct sockaddr_in addr;
            socklen_t len = sizeof(addr);
            if (getpeername(fd, (struct sockaddr*)&addr, &len) == 0)
            {
                return getIPFromSockaddr((const struct sockaddr*)&addr);
            }
            return ""; 
        }  

         std::string getIPFromSockaddr(const struct sockaddr* from)
         {
             return ""; 
         }

}
}