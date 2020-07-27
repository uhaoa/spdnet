#ifndef SPDNET_NET_ENV_INIT_H
#define SPDNET_NET_ENV_INIT_H

#include <csignal>
#include <spdnet/base/platform.h>

namespace spdnet {
    namespace net {

        class env_init {
        public:
            env_init() {
#ifdef SPDNET_PLATFORM_WINDOWS
                static WSADATA gWsaData;
                static bool winSockIsInit = false;
                if (!winSockIsInit)
                {
                    if (WSAStartup(MAKEWORD(2, 2), &gWsaData) == 0){
                        winSockIsInit = true;
                    }
                }

#else
                std::signal(SIGPIPE, SIG_IGN);
#endif
            }

            ~env_init() {
#ifdef SPDNET_PLATFORM_WINDOWS
                WSACleanup();
#endif
            }
        };
    }
}
#endif // SPDNET_NET_ENV_INIT_H
