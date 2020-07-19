#ifndef SPDNET_NET_HTTP_HTTP_SESSION_H_
#define SPDNET_NET_HTTP_HTTP_SESSION_H_

#include <memory>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/http/http_parser.h

namespace spdnet {
    namespace net {
        namespace http {
            class HttpSession : public spdnet::base::noncopyable, public std::enable_shared_from_this<HttpSession> {
            public:
                HttpSession(http_parser_type parser_type);

                ~HttpSession();
            private:
                static int  onChunkHeader(http_parser* parser);
                static int  onChunkComplete(http_parser* parser);
                static int  onMessageBegin(http_parser* parser);
                static int  onMessageEnd(http_parser* parser);
                static int  onHeadComplete(http_parser* parser);
                static int  onUrlHandle(http_parser* parser, const char* url, size_t length);
                static int  onHeadValue(http_parser* parser, const char* at, size_t length);
                static int  onHeadField(http_parser* parser, const char* at, size_t length);
            private:
                http_parser_type parser_type_;
                http_parser_settings setting_;
                bool is_keep_alive_ {false};
                bool is_finish_ {false};
                std::string  current_field_;
                std::string  current_value_;
            };
        }
    }
}

#include <spdnet/net/http/http_session.ipp>

#endif // SPDNET_NET_HTTP_HTTP_SESSION_H_