#include <spdnet/base/current_thread.h>
#include <sys/syscall.h>

namespace spdnet
{
namespace current_thread
{
   __thread int cached_tid = 0 ;
   int& tid()
    {
        if(cached_tid == 0)
        {
            cached_tid = static_cast<int>(syscall(SYS_gettid));
        }
        return cached_tid ; 
    }
}
}