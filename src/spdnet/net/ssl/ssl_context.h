#ifndef SPDNET_NET_SSL_CONTEXT_H_
#define SPDNET_NET_SSL_CONTEXT_H_



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

			class ssl_context : public spdnet::base::noncopyable
			{
			public:
#ifdef ENABLE_OPENSSL
				ssl_context() = default;
				virtual ~ssl_context()
				{
					destroy();
				}


				bool        init_ssl(const std::string& certificate,
					const std::string& privatekey)
				{
					std::call_once(once_flag_ , init_crypto_threadsafe_support);

					if (openssl_ctx_ != nullptr)
					{
						return false;
					}
					if (certificate.empty() || privatekey.empty())
					{
						return false;
					}

					openssl_ctx_ = SSL_CTX_new(SSLv23_method());
					SSL_CTX_set_client_CA_list(openssl_ctx_,
						SSL_load_client_CA_file(certificate.c_str()));
					SSL_CTX_set_verify_depth(openssl_ctx_, 10);

					if (SSL_CTX_use_certificate_chain_file(openssl_ctx_,
						certificate.c_str()) <= 0)
					{
						SSL_CTX_free(openssl_ctx_);
						openssl_ctx_ = nullptr;
						return false;
					}

					if (SSL_CTX_use_PrivateKey_file(openssl_ctx_,
						privatekey.c_str(),
						SSL_FILETYPE_PEM) <= 0)
					{
						SSL_CTX_free(openssl_ctx_);
						openssl_ctx_ = nullptr;
						return false;
					}

					if (!SSL_CTX_check_private_key(openssl_ctx_))
					{
						SSL_CTX_free(openssl_ctx_);
						openssl_ctx_ = nullptr;
						return false;
					}

					return true;
				}

				void        destroy()
				{
					if (openssl_ctx_ != nullptr)
					{
						SSL_CTX_free(openssl_ctx_);
						openssl_ctx_ = nullptr;
					}
				}

				SSL_CTX* get_openssl_ctx()
				{
					return openssl_ctx_;
				}

				static void crypto_set_threadid_callback(CRYPTO_THREADID* id)
				{
					CRYPTO_THREADID_set_numeric(id,
						static_cast<unsigned long>(current_thread::tid()));
				}

				static std::unordered_map<int, std::shared_ptr<std::mutex>> cryptoLocks;
				static void crypto_locking_callback(int mode,
					int type,
					const char* file, int line)
				{
					(void)file;
					(void)line;
					if (mode & CRYPTO_LOCK)
					{
						cryptoLocks[type]->lock();
					}
					else if (mode & CRYPTO_UNLOCK)
					{
						cryptoLocks[type]->unlock();
					}
				}

				
				static void init_crypto_threadsafe_support()
				{
					CRYPTO_THREADID_set_callback(crypto_set_threadid_callback);

					for (int i = 0; i < CRYPTO_num_locks(); i++)
					{
						cryptoLocks[i] = std::make_shared<std::mutex>();
					}
					CRYPTO_set_locking_callback(crypto_locking_callback);
				}

				static std::once_flag once_flag_;
			private:
				SSL_CTX* openssl_ctx_{ nullptr };
#endif
			};

		}
	}
}

#endif // !SPDNET_NET_SSL_CONTEXT_H_