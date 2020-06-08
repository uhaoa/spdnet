#ifndef SPDNET_NET_ENV_INIT_H
#define SPDNET_NET_ENV_INIT_H

#include <csignal>

namespace spdnet::net {

    class EnvInit {
    public:
        EnvInit() {
            std::signal(SIGPIPE, SIG_IGN);
        }
    };

}
#endif // SPDNET_NET_ENV_INIT_H
