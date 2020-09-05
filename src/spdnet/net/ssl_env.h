#ifndef SPDNET_NET_SSL_ENV_H_
#define SPDNET_NET_SSL_ENV_H_
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <spdnet/base/singleton.h>
#include <spdnet/base/platform.h>
#include <spdnet/net/current_thread.h>
#if defined(SPDNET_USE_OPENSSL)
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
#if defined(SPDNET_USE_OPENSSL)
        static void crypto_set_threadid_callback(CRYPTO_THREADID* id)
        {
            CRYPTO_THREADID_set_numeric(id, static_cast<unsigned long>(current_thread::tid()));
        }

        static std::unordered_map<int, std::shared_ptr<std::mutex>> crypto_locks;
        static void crypto_locking_callback(int mode,
            int type,
            const char* file, int line)
        {
            (void)file;
            (void)line;
            if (mode & CRYPTO_LOCK)
            {
                crypto_locks[type]->lock();
            }
            else if (mode & CRYPTO_UNLOCK)
            {
                crypto_locks[type]->unlock();
            }
        }


        static void init_crypto_threadsafe_support()
        {
            CRYPTO_THREADID_set_callback(crypto_set_threadid_callback);

            for (int i = 0; i < CRYPTO_num_locks(); i++)
            {
                crypto_locks[i] = std::make_shared<std::mutex>();
            }
            CRYPTO_set_locking_callback(crypto_locking_callback);
        }

#endif
        class ssl_environment : public spdnet::base::noncopyable
        {
        public:
#if defined(SPDNET_USE_OPENSSL)
            ssl_environment()
            {
            }

            virtual ~ssl_environment()
            {
				if (ssl_ctx_ != nullptr)
				{
					SSL_CTX_free(ssl_ctx_);
					ssl_ctx_ = nullptr;
				}
            }

            bool        init_ssl(const std::string& certificate,
                const std::string& private_key)
            {
                std::call_once(threadsafe_once_flag_,
                    init_crypto_threadsafe_support);

                if (ssl_ctx_ != nullptr || certificate.empty() || private_key.empty())
                    return false;


                ssl_ctx_ = SSL_CTX_new(SSLv23_method());
                SSL_CTX_set_client_CA_list(ssl_ctx_,
                    SSL_load_client_CA_file(certificate.c_str()));
                SSL_CTX_set_verify_depth(ssl_ctx_, 10);

                if (SSL_CTX_use_certificate_chain_file(ssl_ctx_,
                    certificate.c_str()) <= 0)
                {
                    SSL_CTX_free(ssl_ctx_);
                    ssl_ctx_ = nullptr;
                    return false;
                }

                if (SSL_CTX_use_PrivateKey_file(ssl_ctx_,
                    private_key.c_str(),
                    SSL_FILETYPE_PEM) <= 0)
                {
                    SSL_CTX_free(ssl_ctx_);
                    ssl_ctx_ = nullptr;
                    return false;
                }

                if (!SSL_CTX_check_private_key(ssl_ctx_))
                {
                    SSL_CTX_free(ssl_ctx_);
                    ssl_ctx_ = nullptr;
                    return false;
                }

                return true;
            }

            SSL_CTX* get_ssl_ctx()
            {
                return ssl_ctx_;
            }

        private:
            static std::once_flag threadsafe_once_flag_;
            SSL_CTX* ssl_ctx_{ nullptr };

#endif
        };
#if defined(SPDNET_USE_OPENSSL)
        std::once_flag ssl_environment::threadsafe_once_flag_;
#endif
    }
}


#endif // SPDNET_NET_SSL_ENV_H_