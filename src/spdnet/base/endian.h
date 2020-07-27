#ifndef SPDNET_BASE_ENDIAN_H
#define SPDNET_BASE_ENDIAN_H
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdbool>
#include <cstring>
#include <spdnet/base/platform.h>

namespace spdnet {
    namespace base {
        namespace util {

            inline uint64_t hl64ton(uint64_t num) {
                uint64_t ret = 0;
                uint32_t high, low;

                low = num & 0xFFFFFFFF;
                high = (num >> 32) & 0xFFFFFFFF;
                low = htonl(low);
                high = htonl(high);
                ret = low;
                ret <<= 32;
                ret |= high;

                return ret;
            }

            inline uint64_t ntohl64(uint64_t num) {
                uint64_t ret = 0;
                uint32_t high, low;

                low = num & 0xFFFFFFFF;
                high = (num >> 32) & 0xFFFFFFFF;
                low = ntohl(low);
                high = ntohl(high);
                ret = low;
                ret <<= 32;
                ret |= high;

                return ret;
            }

#ifdef SPDNET_PLATFORM_WINDOWS
            inline uint64_t host_to_net_64(uint64_t num)
            {
                return  hl64ton(num);
            }
            inline uint32_t host_to_net_32(uint32_t num)
            {
                return htonl(num) ;
            }

            inline uint16_t host_to_net_16(uint16_t num)
            {
                return htons(num);
            }

            inline uint64_t net_to_host_64(uint64_t num)
            {
                return ntohl64(num);
            }

            inline uint32_t net_to_host_32(uint32_t num)
            {
                return ntohl(num);
            }

            inline uint16_t net_to_host_16(uint16_t num)
            {
                return ntohs(num);
            }
#else

            inline uint64_t host_to_net_64(uint64_t num) {
                return htobe64(num);
            }

            inline uint32_t host_to_net_32(uint32_t num) {
                return htobe32(num);
            }

            inline uint16_t host_to_net_16(uint16_t num) {
                return htobe16(num);
            }

            inline uint64_t net_to_host_64(uint64_t num) {
                return be64toh(num);
            }

            inline uint32_t net_to_host_32(uint32_t num) {
                return be32toh(num);
            }

            inline uint16_t net_to_host_16(uint16_t num) {
                return be16toh(num);
            }

#endif

        }
    }
}

#endif // SPDNET_BASE_ENDIAN_H