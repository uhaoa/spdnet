#ifndef SPDNET_NET_EXCEPTION_H_
#define SPDNET_NET_EXCEPTION_H_

#include <string>
#include <exception>
#include <stdexcept>

namespace spdnet {
    namespace net {
        class spdnet_exception : public std::runtime_error {
        public:
            explicit spdnet_exception(const std::string &msg)
                    : std::runtime_error(msg) {

            }

            explicit spdnet_exception(const char *msg)
                    : std::runtime_error(msg) {

            }

        };

    }
}
#endif      // SPDNET_NET_EXCEPTION_H_    