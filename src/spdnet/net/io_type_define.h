#ifndef SPDNET_NET_IO_TYPE_DEFINE_H
#define SPDNET_NET_IO_TYPE_DEFINE_H

#include <spdnet/base/platform.h>
#include <functional>
namespace spdnet {
	namespace net {
		namespace detail {
			class EpollImpl;
			class IocpImpl;
		}

#if defined SPDNET_PLATFORM_LINUX
		using IoObjectImplType = detail::EpollImpl;
#else
		using IoObjectImplType = detail::IocpImpl;
#endif
	}
}
#endif // SPDNET_NET_IO_TYPE_DEFINE_H
