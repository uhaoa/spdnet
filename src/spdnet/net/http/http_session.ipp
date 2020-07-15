#ifndef SPDNET_NET_HTTP_HTTP_SESSION_IPP_
#define SPDNET_NET_HTTP_HTTP_SESSION_IPP_

#include <memory>
#include <spdnet/net/http/http_session.h>

namespace spdnet {
    namespace net {
        namespace http {
///////////////////////////////////////////////////////////////////////////
            static int  onChunkHeader(http_parser* hp)
            {
                return 0;
            }

            static int  onChunkComplete(http_parser* parser)
            {
                return 0;
            }

            static int  onMessageBegin(http_parser* parser)
            {
                HttpSession* session = (HttpSession*)hp->data;
                session->current_field_.clear();
                session->current_value_.clear();

                return 0;
            }

            static int  onMessageEnd(http_parser* parser)
            {
                HttpSession* httpParser = (HttpSession*)parser->data;
                session->is_finish_ = true;
                return 0;
            }

            static int  onHeadComplete(http_parser* parser)
            {



                return 0;
            }

            static int  onUrlHandle(http_parser* parser, const char* url, size_t length)
            {

                return 0;
            }

            static int  onHeadValue(http_parser* parser, const char* at, size_t length)
            {

                return 0;
            }

            static int  onHeadField(http_parser* parser, const char* at, size_t length)
            {


                return 0;
            }

            static int  onStatusHandle(http_parser* parser, const char* at, size_t length)
            {

                return 0;
            }

            static int  onBodyHandle(http_parser* parser, const char* at, size_t length)
            {

                return 0;
            }
////////////////////////////////////////////////////////////////////////

            HttpSession::HttpSession(http_parser_type parser_type)
                :parser_type_(parser_type)
            {
                setting_.on_status = onStatusHandle;
                setting_.on_body = onBodyHandle;
                setting_.on_url = onUrlHandle;
                setting_.on_header_field = onHeadField;
                setting_.on_header_value = onHeadValue;
                setting_.on_headers_complete = onHeadComplete;
                setting_.on_message_begin = onMessageBegin;
                setting_.on_message_complete = onMessageEnd;
                setting_.on_chunk_header = onChunkHeader;
                setting_.on_chunk_complete = onChunkComplete;
            }
        }
    }
}



#endif // SPDNET_NET_HTTP_HTTP_SESSION_IPP_