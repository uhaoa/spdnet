#ifndef SPDNET_NET_HTTP_HTTP_PARSER_API_H_
#define SPDNET_NET_HTTP_HTTP_PARSER_API_H_

#include <memory>
#include <map>
#include <sstream>
#include <spdnet/base/singleton.h>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/http/http_parser.h>

namespace spdnet {
    namespace net {
        namespace http {
            template<typename Parser>
            class parser_setting : public spdnet::base::singleton<parser_setting<Parser>> {
            public:
                parser_setting() {
                    setting_.on_message_begin = on_message_begin;
                    setting_.on_status = on_status;
                    setting_.on_body = on_body;
                    setting_.on_url = on_url;
                    setting_.on_header_field = on_header_field;
                    setting_.on_header_value = on_header_value;
                    setting_.on_headers_complete = on_headers_complete;
                    setting_.on_message_complete = on_message_complete;
                    setting_.on_chunk_header = on_chunk_header;
                    setting_.on_chunk_complete = on_chunk_complete;

                    http_parser_settings_init(&setting_);
                }

                http_parser_settings &get_setting() { return setting_; }

            public:
                template<typename Func, typename ... Args>

                static int call(http_parser *parser, Func func, Args ... args) {
                    Parser *p = (Parser *) parser->data;
                    return (p->*func)(args...);
                }

                static int on_message_begin(http_parser *p) noexcept { return call(p, &Parser::on_message_begin); }

                static int on_url(http_parser *p, const char *data, size_t length) noexcept {
                    return call(p, &Parser::on_url, data, length);
                }

                static int on_status(http_parser *p, const char *data, size_t length) noexcept {
                    return call(p, &Parser::on_status, data, length);
                }

                static int on_header_field(http_parser *p, const char *data, size_t length) noexcept {
                    return call(p, &Parser::on_header_field, data, length);
                }

                static int on_header_value(http_parser *p, const char *data, size_t length) noexcept {
                    return call(p, &Parser::on_header_value, data, length);
                }

                static int on_headers_complete(http_parser *p) noexcept {
                    return call(p, &Parser::on_headers_complete);
                }

                static int on_message_complete(http_parser *p) noexcept { return call(p, &Parser::on_message_complete); }

                static int on_body(http_parser *p, const char *data, size_t length) noexcept {
                    return call(p, &Parser::on_body, data, length);
                }

                static int on_chunk_header(http_parser *p) noexcept { return call(p, &Parser::on_chunk_header); }

                static int on_chunk_complete(http_parser *p) noexcept { return call(p, &Parser::on_chunk_complete); }

            private:
                http_parser_settings setting_;
            };

            class http_header_set {
            public:
                void add_header(const std::string &name, const std::string &value) {
                    std::string &current_value = headers_[name];
                    if (current_value.empty()) {
                        current_value = value;
                    } else if (!value.empty()) {
                        current_value += ",";
                        current_value += value;
                    }
                }

                bool has_header(const std::string &name) {
                    return headers_.count(name) > 0;
                }

                const std::string &get_header(const std::string &name) {
                    const static std::string null_str;
                    const auto iter = headers_.find(name);
                    if (iter == headers_.end()) {
                        return null_str;
                    }
                    return iter->second;
                }

                const std::map<std::string, std::string> &headers() const {
                    return headers_;
                }

            protected:
                void reset() {
                    headers_.clear();
                }

            protected:
                std::map<std::string, std::string> headers_;
            };

            class http_body {
            public:
                void set_body(const std::string &body) {
                    body_ = body;
                }

                const std::string &get_body() const {
                    return body_;
                }

                void append_body(const char *data, size_t len) {
                    body_.append(data, len);
                }

            protected:
                void reset() {
                    body_.clear();
                }

            protected:
                std::string body_;
            };

            class http_version {
            public:
                http_version(uint16_t major , uint16_t minor)
                    :major_(major) , minor_(minor)
                {}
                http_version() = default;
                http_version(const http_version&) = default;
                http_version(http_version&&) = default;
                http_version& operator=(const http_version&) = default;
                http_version& operator=(http_version&& ) = default;

                uint16_t get_major() const { return major_; }
                uint16_t get_minor() const { return minor_; }

                std::string to_string() const {
                    std::string ret;
                    ret += major_;
                    ret += ".";
                    ret += minor_;
                    return ret;
                }

                void reset() {
                    major_ = 0;
                    minor_ = 0;
                }

            private:
                uint16_t major_{0};
                uint16_t minor_{0};
            };

            class http_url_info {
            public:
                friend class http_request;

                std::string to_string() const  {
                    std::ostringstream oss;
                    oss << "URL\n"
                        << "schema: '" << schema_ << "'\n"
                        << "host: '" << host_ << "'\n"
                        << "port: " << port_ << "\n"
                        << "path: '" << path_ << "'\n"
                        << "query: '" << query_ << "'\n"
                        << "fragment: '" << fragment_ << "'\n"
                        << "userinfo: '" << userinfo_ << "'\n";

                    return oss.str();
                }

                void parse(const std::string& input, bool is_connect = false) {
                    http_parser_url u;
                    http_parser_url_init(&u);
                    int err = http_parser_parse_url(
                            input.c_str(), input.length(), int(is_connect), &u);
                    if (err) {
                        return ;
                       // throw url_parse_error("Failed to parse this url: '" + input + "'");
                    }
                    using field_t = std::pair<http_parser_url_fields, std::string http_url_info::*>;
                    static const field_t string_fields[] = {
                            {UF_SCHEMA,   &http_url_info::schema_},
                            {UF_HOST,     &http_url_info::host_},
                            {UF_PATH,     &http_url_info::path_},
                            {UF_QUERY,    &http_url_info::query_},
                            {UF_FRAGMENT, &http_url_info::fragment_},
                            {UF_USERINFO, &http_url_info::userinfo_}
                    };
                    for (const field_t &field : string_fields) {
                        if (u.field_set & (1 << field.first)) {
                            this->*field.second = input.substr(
                                    u.field_data[field.first].off, u.field_data[field.first].len);
                        }
                    }

                    if (u.field_set & (1 << UF_PORT)) {
                        port_ = std::stoul(input.substr(
                                u.field_data[UF_PORT].off, u.field_data[UF_PORT].len
                        ));
                    }
                }

                const std::string& get_schema() const {return schema_ ; }
                const std::string& get_host() const {return host_ ; }
                const std::string& get_path() const {return path_ ; }
                const std::string& get_query() const {return query_ ; }
                const std::string& get_fragment() const { return fragment_ ; }
                const std::string& get_userinfo() const {return userinfo_ ; }
                uint16_t get_port() const {return port_; }

                void set_schema(const std::string& str) { schema_ = str ; }
                void set_host(const std::string& str) { host_ = str ; }
                void set_path(const std::string& str) { path_ = str ; }
                void set_query(const std::string& str) { query_ = str ; }
                void set_fragment(const std::string& str) { fragment_ = str ; }
                void set_userinfo(const std::string& str) { userinfo_ = str ; }
                void set_port(uint16_t port) { port_ = port; }

                void reset()
                {
                    schema_.clear();
                    host_.clear();
                    path_.clear();
                    query_.clear();
                    fragment_.clear();
                    userinfo_.clear();
                    port_ = 0;
                }
            private:
                std::string schema_;
                std::string host_;
                std::string path_;
                std::string query_;
                std::string fragment_;
                std::string userinfo_;
                uint16_t port_ = 0;
            };


            class http_request : public http_header_set, public http_body {
            public:
                http_request() = default;
                void reset() {
                    http_header_set::reset();
                    http_body::reset();
                    version_.reset();
                    method_ = HTTP_HEAD;
                    keep_alive_ = false;
                    url_.clear();
                    url_info_.reset();
                }

                http_method get_method() const {
                    return method_;
                }

                void set_method(http_method method) {
                    method_ = method;
                }

                bool is_keep_alive() const {
                    return keep_alive_;
                }

                void set_keep_alive(bool keep_alive) {
                    keep_alive_ = keep_alive;
                }

                const std::string &get_url_str() const { return url_; }

                const http_url_info& get_url_info() const { return url_info_; }

                http_url_info& get_url_info() { return url_info_; }

                void set_url_str(const std::string &url) {
                    url_ = url;
                }

                void append_url_str(const char *data, size_t len) {
                    url_.append(data, len);
                }

                const http_version& get_version() const {return version_;}
                void set_version(const http_version& version) {version_ = version;}

                std::string to_string()
                {
                    std::ostringstream  oss;
                    if (url_.empty()) {
                        url_ =url_info_.to_string();
                    }
                    oss << "HTTP/" << version_.to_string() << " " << http_method_str(method_) << " request.\n"
                           << "\tUrl: '" << url_ << "'\n"
                           << "\tHeaders:\n";
                    for (const auto& pair : headers_) {
                        oss << "\t\t'" << pair.first << "': '" << pair.second << "'\n";
                    }
                    oss << "\tBody is " << body_.size() << " bytes long.\n\tKeepAlive: "
                           << (keep_alive_ ? "yes" : "no") << ".";

                    return std::string(oss.str());
                }

                void add_query_param(const std::string& key , const std::string& val)
                {
                    if (!url_info_.query_.empty()) {
                        url_info_.query_ += "&";
                    }
                    url_info_.query_ += key;
                    url_info_.query_ += "=";
                    url_info_.query_ += val;
                }
            private:
                http_method method_ = HTTP_HEAD;
                bool keep_alive_ = false;
                std::string url_;
                http_url_info url_info_;
                http_version version_;
            };

            class http_parser_base {
            protected:
                http_parser_base(http_parser_type type, http_parser_settings &parser_settings)
                        : parser_settings_(parser_settings) {
                    http_parser_init(&parser_, type);
                    parser_.data = this;
                }

            public:
                size_t try_parse(const char *data, size_t len) {
                    const size_t nparsed = http_parser_execute(&parser_, &parser_settings_, data, len);
                    return nparsed;
                }

            protected:
                http_parser parser_;
                http_parser_settings &parser_settings_;
            };

            class http_header_parser {
            public:
                http_header_parser(http_header_set &header)
                        : header_(header) {

                }

                int on_header_field(const char *data, size_t len) {
                    if (current_header_field_complete_) {
                        try_add_current_header();
                    }
                    current_header_field_.append(data, len);
                    return 0;
                }

                int on_header_value(const char *data, size_t len) {
                    current_header_value_.append(data, len);
                    current_header_field_complete_ = true;
                    return 0;
                }

                int on_chunk_header() {
                    reset();
                    return 0;
                }

                int on_chunk_complete() {
                    try_add_current_header();
                    return 0;
                }

                int on_headers_complete() {
                    try_add_current_header();
                    return 0;
                }

                void reset() {
                    current_header_field_.clear();
                    current_header_value_.clear();
                    current_header_field_complete_ = false;
                }

            private:
                void try_add_current_header() {
                    if (current_header_field_.empty())
                        return;
                    header_.add_header(current_header_field_, current_header_value_);
                    reset();
                }

            private:
                std::string current_header_field_;
                std::string current_header_value_;
                bool current_header_field_complete_ = false;
                http_header_set &header_;
            };

            class http_request_parser : public http_parser_base, public http_header_parser, public spdnet::base::noncopyable {
            public:
				using request_complete_callback = std::function<void(const http_request&)>;
			public:
                http_request_parser()
                        : http_parser_base(HTTP_REQUEST, parser_setting<http_request_parser>::instance().get_setting()), http_header_parser(request_) {}

				void set_parse_complete_callback(request_complete_callback&& cb) {
					complete_callback_ = std::move(cb);
				}
                int on_message_begin() {
                    request_.reset();
                    http_header_parser::reset();
                    return 0;
                }

                int on_url(const char *data, size_t len) {
                    request_.append_url_str(data, len);
                    return 0;
                }

                int on_status(const char *data, size_t len) {
                    assert(false);
                    return 0;
                }

                int on_body(const char *data, size_t len) {
                    request_.append_body(data, len);
                    return 0;
                }

                int on_message_complete() {
                    request_.set_method(static_cast<http_method>(parser_.method));
                    request_.set_version(http_version(parser_.http_major , parser_.http_minor));
                    request_.set_keep_alive(http_should_keep_alive(&parser_) != 0);
					if (complete_callback_){
						complete_callback_(request_);
					}
                    request_.reset();
                    return 0;
                }

            private:
                http_request request_;
				request_complete_callback complete_callback_;
            };
            /*
            class http_response_parser : public http_parser_base, http_header_parser, spdnet::base::noncopyable {
            public:
                http_response_parser()
                        : http_parser_base(HTTP_RESPONSE, parser_settings_<http_response_parser>::instance()) {}
            };
            */
        }
    }
}


#endif // SPDNET_NET_HTTP_HTTP_PARSER_API_H_