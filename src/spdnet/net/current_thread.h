#ifndef SPDNET_NET_CURRENT_THREAD_H_
#define SPDNET_NET_CURRENT_THREAD_H_

#include <spdnet/base/platform.h>

#if defined SPDNET_PLATFORM_WINDOWS
#include <winsock2.h>
#include <Windows.h>
#elif defined SPDNET_PLATFORM_LINUX

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

#endif

namespace spdnet::net::current_thread {
#if defined SPDNET_PLATFORM_WINDOWS
    using THREAD_ID_TYPE = DWORD;
#elif defined SPDNET_PLATFORM_LINUX
    using THREAD_ID_TYPE = int;
#endif

    inline THREAD_ID_TYPE &tid() {
        static THREAD_LOCAL THREAD_ID_TYPE cached_tid = 0;
        if (cached_tid == 0) {
#if defined SPDNET_PLATFORM_WINDOWS
            cachedTid = ::GetCurrentThreadId();
#elif defined SPDNET_PLATFORM_LINUX
            cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
#endif
        }
        return cached_tid;
    }
}

#endif //SPDNET_NET_CURRENT_THREAD_H_