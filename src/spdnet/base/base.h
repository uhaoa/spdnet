#ifndef SPDNET_BASE_BASE_H
#define SPDNET_BASE_BASE_H

#include<netdb.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<sys/uio.h>
#include<sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

    //#define sErrno errno
    //#define SOCKET_ERROR (-1)
    //#define  (-1)
    //using sock = int ; 
    //using THREAD_ID_T = int ; 



#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

#endif  // SPDNET_BASE_BASE_H