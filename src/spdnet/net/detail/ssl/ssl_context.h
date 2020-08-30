#ifndef SPDNET_NET_DETAIL_SSL_CONTEXT_H_
#define SPDNET_NET_DETAIL_SSL_CONTEXT_H_
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>

#include <spdnet/base/noncopyable.h>
#include <spdnet/base/platform.h>
#if defined (SPDNET_USE_OPENSSL)
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
		namespace detail {
			class ssl_context : public spdnet::base::noncopyable
            {
            public:
                ssl_context(sock_t sockfd)
					:fd_(sockfd)
				{

				}
#if defined (SPDNET_USE_OPENSSL)
				virtual ~ssl_context()
				{
					if (ssl_ != nullptr) {
						SSL_free(ssl_);
						ssl_ = nullptr;
					}
					if (ssl_ctx_ != nullptr) {
						SSL_CTX_free(ssl_ctx_);
						ssl_ctx_ = nullptr;
					}
				}
				bool init_server_side(SSL_CTX* ctx)
				{
					if (ssl_ != nullptr || ctx == nullptr)
						return false;

					ssl_ = SSL_new(ctx);
					if (SSL_set_fd(ssl_, fd_) != 1)
					{
						ERR_print_errors_fp(stdout);
						::fflush(stdout);
						return false;
					}

					return true;
				}
				bool init_client_side()
				{
					if (ssl_ctx_ != nullptr)
						return false;

					ssl_ctx_ = SSL_CTX_new(SSLv23_client_method());
					ssl_ = SSL_new(ssl_ctx_);

					if (SSL_set_fd(ssl_, fd_) != 1)
					{
						ERR_print_errors_fp(stdout);
						::fflush(stdout);
						return false;
					}

					return true;
				}

				bool try_start_ssl_handshake()
				{
					if (has_already_handshake_)
						return true;

					bool mustClose = false;
					int ret = 0;

					if (ssl_ctx_ != nullptr)
						ret = SSL_connect(ssl_);
					else
						ret = SSL_accept(ssl_);

					if (ret == 0)
						return false; 
					if (ret == 1) {
						has_already_handshake_ = true;
					}
					else if (ret < 0)
					{
						int err = SSL_get_error(ssl_, ret);
						if (err != SSL_ERROR_WANT_WRITE && err != SSL_ERROR_WANT_READ) {
							return false;
						}
					}
					return true;
				}

				bool has_already_handshake() const { return has_already_handshake_;  }

				SSL* get_ssl() const { return ssl_;  }
            private:
				SSL_CTX* ssl_ctx_{nullptr};
				SSL* ssl_{ nullptr };
				bool has_already_handshake_{ false };
#endif
				sock_t fd_{ invalid_socket };
            };

        }
    }
}


#endif // SPDNET_NET_DETAIL_SSL_CONTEXT_H_