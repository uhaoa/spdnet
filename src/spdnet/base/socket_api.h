#ifndef SPDNET_BASE_SOCKETAPI_H_
#define SPDNET_BASE_SOCKETAPI_H_
#include "base.h"
#include <string>  

namespace spdnet 
{
    namespace base
    {
        /**
		 * \brief 初始化
		 * \param 
		 * \return 
		*/
        bool InitSocketEnv() ; 
        /**
		 * \brief 禁用Nagle算法
		 * \param fd
		 * \return 
		*/
        int  SetSocketNoDelay(int fd); 
        /**
		 * \brief 设置阻塞模式
		 * \param fd
		 * \return 
		*/
        bool SetSocketBlock(int fd);
        /**
		 * \brief 设置非阻塞模式
		 * \param fd
		 * \return 
		*/
        bool SetSocketNonBlock(int fd);
        /**
		 * \brief 设置socket内核发送缓冲区大小
		 * \param fd
		 * \return 
		*/
        int SetSocketSendBufSize(int fd , int size) ;
        /**
		 * \brief 设置socket内核接受缓冲区大小
		 * \param fd
		 * \return 
		*/
        int SetSocketRecvBufSize(int fd , int size) ;
        /**
		 * \brief 关闭socket描述符
		 * \param fd
		 * \return 
		*/
        void CloseSocket(int fd);

        /**
		 * \brief 获取sock的ip字符串
		 * \param 
		 * \return ip
		*/
        std::string getIPFromSockaddr(const struct sockaddr* from) ; 
        /**
		 * \brief 获取sock的ip字符串
		 * \param 
		 * \return ip
		*/
        std::string getIPFromSockFd(int fd) ;

    
    }
}

#endif // SPDNET_BASE_SOCKETAPI_H_