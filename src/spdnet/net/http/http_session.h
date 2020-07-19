#ifndef SPDNET_NET_HTTP_HTTP_SESSION_H_
#define SPDNET_NET_HTTP_HTTP_SESSION_H_

#include <memory>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/http/http_parser.h>

namespace spdnet {
    namespace net {
        namespace http {
            class http_session : public spdnet::base::noncopyable, public std::enable_shared_from_this<HttpSession> {
            public:
                http_session(std::shared_ptr<tcp_session>);

                ~http_session();
            private:
            private:
                std::shared_ptr<tcp_session> session_;
            };
        }
    }
}

#include <spdnet/net/http/http_session.ipp>

#endif // SPDNET_NET_HTTP_HTTP_SESSION_H_