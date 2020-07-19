#ifndef SPDNET_NET_HTTP_HTTP_SESSION_IPP_
#define SPDNET_NET_HTTP_HTTP_SESSION_IPP_

#include <memory>
#include <spdnet/net/http/http_session.h>

namespace spdnet {
    namespace net {
        namespace http
        {
            http_session::http_session(std::shared_ptr<tcp_session> session)
                :session_(session)
            {}


            http_session::~http_session()
            {}

        }
    }
}



#endif // SPDNET_NET_HTTP_HTTP_SESSION_IPP_