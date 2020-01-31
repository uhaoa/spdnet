#ifndef SPDNET_BASE_CURRENT_THREAD_H_
#define SPDNET_BASE_CURRENT_THREAD_H_
#include <spdnet/base/base.h>

namespace spdnet
{
namespace current_thread
{
    extern __thread int cached_tid ;
	int& tid();
}
}

#endif //SPDNET_BASE_CURRENT_THREAD_H_