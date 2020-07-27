#ifndef SPDNET_NET_SSL_SESSION_H_
#define SPDNET_NET_SSL_SESSION_H_



#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>

#ifdef ENABLE_OPENSSL

#ifdef  __cplusplus
extern "C" {
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#ifdef  __cplusplus
}
#endif

#endif

namespace spdnet {
	namespace net {
		namespace ssl {

			class ssl_session : public spdnet::base::noncopyable
			{
			public:
				ssl_session() = default;
				virtual ~ssl_session() = default;
			};

		}
	}
}

#endif // SPDNET_NET_SSL_SESSION_H_