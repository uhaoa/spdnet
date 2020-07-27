#ifndef SPDNET_NET_HTTP_WEBSOCKET_PARSER_H_
#define SPDNET_NET_HTTP_WEBSOCKET_PARSER_H_

#include <memory>
#include <sstream>
#include <iostream>
#include <functional>
#include <spdnet/base/singleton.h>
#include <spdnet/base/noncopyable.h>
#include <spdnet/base/endian.h>

namespace spdnet {
    namespace net {
        namespace http {
/******************************************************************************************
            0                   1                   2                   3
            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
            +-+-+-+-+-------+-+-------------+-------------------------------+
            |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
            |I|S|S|S|  (4)  |A|     (7)     |            (16/64)            |
            |N|V|V|V|       |S|             |   (if payload len==126/127)   |
            | |1|2|3|       |K|             |                               |
            +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
            |    Extended payload length continued, if payload len == 127   |
            + - - - - - - - - - - - - - - - +-------------------------------+
            |                               | Masking-key, if MASK set to 1 |
            +-------------------------------+-------------------------------+
            |    Masking-key (continued)    |          Payload Data         |
            +-------------------------------- - - - - - - - - - - - - - - - +
            :                   Payload Data continued ...                  :
            + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
            |                   Payload Data continued ...                  |
            +---------------------------------------------------------------+
******************************************************************************************/

            enum class ws_opcode {
                op_continuation_frame = 0x00,
                op_text_frame = 0x01,
                op_binary_frame = 0x02,
                op_close_frame = 0x08,
                op_ping_frame = 0x09,
                op_pong_frame = 0x0A,
                /////////////////////////
                op_handshake_req,
                op_handshake_ack,
                op_unknow
            };

            class websocket_frame {
            public:
                friend class http_session;

                friend class websocket_parser;

                websocket_frame() = default;

                websocket_frame(const websocket_frame &) = default;

                websocket_frame &operator=(const websocket_frame &) = default;

                websocket_frame(websocket_frame &&) = default;

                websocket_frame &operator=(websocket_frame &&) = default;

                websocket_frame(const std::string &payload, ws_opcode opcode, bool fin, bool mask)
                        : payload_(payload), opcode_(opcode), fin_(fin), mask_(mask) {

                }

                websocket_frame(const char *payload, size_t len, ws_opcode opcode, bool fin, bool mask)
                        : payload_(payload, len), opcode_(opcode), fin_(fin), mask_(mask) {

                }

                ws_opcode get_opcode() const { return opcode_; }

                void set_opcode(ws_opcode opcode) {
                    opcode_ = opcode;
                }

                const std::string &get_payload() const { return payload_; }

                void set_payload(const std::string &payload) { payload_ = payload; }

                void set_payload(const char *payload, size_t len) {
                    payload_.clear();
                    payload_.append(payload, len);
                }

                void append_payload(const std::string &payload) { payload_ += payload; }

                void append_payload(const char *payload, size_t len) {
                    payload_.append(payload, len);
                }

                void set_fin(bool fin) { fin_ = fin; }

                void set_mask(bool mask) { mask_ = mask; }


                void reset() {
                    opcode_ = ws_opcode::op_unknow;
                    payload_.clear();
                    ws_key_.clear();
                    fin_ = true;
                    mask_ = true;
                }

                std::string to_string() const {
                    std::string result;
                    static std::mt19937 random(time(0));
                    result.clear();
                    // max head length = 14
                    result.resize(14 + payload_.length());
                    uint8_t *buf = (uint8_t *) (const_cast<char *>(result.data()));
                    uint32_t pos = 0;
                    buf[pos++] = static_cast<uint8_t>(opcode_) | (fin_ ? 0x80 : 0x00);

                    if (payload_.length() <= 125) {
                        buf[pos++] = static_cast<uint8_t>(payload_.length());
                    } else if (payload_.length() <= 0xFFFF) {
                        buf[pos++] = 126;
                        *(uint16_t *) (&buf[pos]) = spdnet::base::util::host_to_net_16(payload_.length());
                        pos += 2;
                    } else {
                        buf[pos++] = 127;
                        *(uint64_t *) (&buf[pos]) = spdnet::base::util::host_to_net_64(payload_.length());
                        pos += 8;
                    }

                    if (mask_) {
                        buf[1] |= 0x80;
                        for (size_t i = 0; i < 4; i++)
                            buf[pos + i] = static_cast<uint8_t>(random());
                        for (size_t i = 0; i < payload_.length(); i++)
                            buf[pos + 4 + i] = static_cast<uint8_t>(payload_[i]) ^ buf[pos + i % 4];

                        pos += (4 + payload_.length());
                    } else {
                        memcpy(buf + pos, payload_.c_str(), payload_.length());
                    }
                    result.resize(pos + payload_.length());
                    return result;
                }

            private:
                std::string payload_;
                ws_opcode opcode_ = ws_opcode::op_unknow;
                std::string ws_key_;
                bool fin_{true};
                bool mask_{true};
            };


            class websocket_parser {
            public:
                using ws_frame_complete_callback = std::function<void(websocket_frame &)>;

                size_t try_ws_parse(const char *data, size_t len) {
                    if (!sec_websocket_key_.empty()) {
                        // handshake frame
                        frame_.opcode_ = ws_opcode::op_handshake_req;
                        frame_.ws_key_ = std::move(sec_websocket_key_);
                        if (callback_ != nullptr)
                            callback_(frame_);
                        frame_.reset();
                    } else if (!sec_websocket_accept_.empty()) {
                        // handshake frame
                        frame_.opcode_ = ws_opcode::op_handshake_ack;
                        if (callback_ != nullptr)
                            callback_(frame_);
                        frame_.reset();
                        sec_websocket_accept_.clear();
                    }
                    size_t left_len = len;
                    while (left_len > 0) {
                        bool fin_flag;
                        std::string payload;
                        ws_opcode opcode;
                        size_t frame_size = 0;
                        if (!try_parse_frame(data, left_len, fin_flag, payload, opcode, frame_size))
                            break;
                        assert(frame_size > 0);
                        frame_.payload_ += payload;
                        if (opcode != ws_opcode::op_continuation_frame)
                            frame_.opcode_ = opcode;
                        assert(left_len >= frame_size);
                        data += frame_size;
                        left_len -= frame_size;

                        if (!fin_flag)
                            continue;

                        if (callback_ != nullptr)
                            callback_(frame_);

                        frame_.reset();
                    }
                    assert(len >= left_len);
                    return len - left_len;
                }

                void set_ws_frame_complete_callback(ws_frame_complete_callback &&callback) {
                    callback_ = std::move(callback);
                }

                void set_sec_websocket_key(const std::string &str) {
                    sec_websocket_key_ = str;
                }

                void set_sec_websocket_accept(const std::string &str) {
                    sec_websocket_accept_ = str;
                }

            private:
                bool
                try_parse_frame(const char *data, size_t len, bool &fin_flag, std::string &payload, ws_opcode &opcode,
                                size_t &frame_size) {
                    int pos = 0;
                    auto buf = (const uint8_t *) data;
                    if (len < 2)
                        return false;

                    fin_flag = buf[pos] & 0x80 ? true : false;
                    opcode = static_cast<ws_opcode>(buf[pos++] & 0x0F);
                    bool mask_flag = buf[pos] & 0x80 ? true : false;
                    uint64_t payload_len = buf[pos++] & 0x7F;
                    if (payload_len == 126) {
                        if (len < 4)
                            return false;
                        uint16_t tmp_len = 0;
                        memcpy(&tmp_len, buf + pos, 2);
                        payload_len = spdnet::base::util::net_to_host_16(tmp_len);
                        pos += 2;
                    } else if (payload_len == 127) {
                        if (len < 10)
                            return len;
                        uint64_t tmp_len = 0;
                        memcpy(&tmp_len, buf + pos, 8);
                        payload_len = spdnet::base::util::net_to_host_64(tmp_len);
                        pos += 8;
                    }
                    uint8_t mask_buf[4];
                    if (mask_flag) {
                        if (len < pos + 4) {
                            return false;
                        }
                        std::copy(buf + pos, buf + pos + 4, mask_buf);
                        pos += 4;
                    }
                    if (len < pos + payload_len)
                        return false;

                    if (mask_flag) {
                        payload.reserve(payload_len);
                        for (size_t i = 0; i < payload_len; i++)
                            payload.push_back(buf[pos + i] ^ mask_buf[i % 4]);
                    } else {
                        payload.append((const char *) (buf + pos), payload_len);
                    }

                    frame_size = payload_len + pos;
                    return true;
                }

            private:
                std::string sec_websocket_key_;
                std::string sec_websocket_accept_;
                websocket_frame frame_;
                ws_frame_complete_callback callback_;
            };

        }
    }
}


#endif // SPDNET_NET_HTTP_WEBSOCKET_PARSER_H_