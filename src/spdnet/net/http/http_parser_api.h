#ifndef SPDNET_NET_HTTP_HTTP_PARSER_API_H_
#define SPDNET_NET_HTTP_HTTP_PARSER_API_H_

#include <memory>
#include <map>
#include <sstream>
#include <iostream>
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
					http_parser_settings_init(&setting_);
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
                    std::ostringstream oss; 
                    oss << major_ << "." << minor_; 
                    return oss.str();
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
				http_request(const http_request&) = default;
				http_request& operator=(const http_request&) = default;
				http_request(http_request&&) = default;
				http_request& operator=(http_request&&) = default;

                void reset() {
                    http_header_set::reset();
                    http_body::reset();
                    version_.reset();
                    method_ = HTTP_HEAD;
                    keep_alive_ = false;
                    url_.clear();
                    parsed_url_info_.reset();
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

                const std::string &get_url() const { return url_; }

                const http_url_info& get_parsed_url_info() const { return parsed_url_info_; }

                void set_url(const std::string &url) {
                    url_ = url;
                }

                void append_url_str(const char *data, size_t len) {
                    url_.append(data, len);
                }

                const http_version& get_version() const { return version_;}
                void set_version(const http_version& version) {version_ = version;}

                std::string to_string() const 
                {
                    std::ostringstream  oss;
                    /*
                    if (url_.empty()) {
                        url_ =url_info_.to_string();
                    }
                    */
                    oss << http_method_str(method_) << " " << url_; 
                    if (!parsed_url_info_.query_.empty()) {
                        oss << "?";
                        oss << parsed_url_info_.query_;
                    }
                    oss <<  " HTTP/" << version_.to_string();
                    oss << "\r\n"; 
					if (!body_.empty()) {
						const_cast<http_request&>(*this).add_header("Content-Length", std::to_string(body_.size()));
					}
                    for (const auto& pair : headers_) {
                        oss <<  pair.first << ": " << pair.second << "\r\n";
                    }
                    oss << "\r\n";
                    oss << body_; 

                    return oss.str();
                }

                void add_query_param(const std::string& key , const std::string& val)
                {
                    if (!parsed_url_info_.query_.empty()) {
                        parsed_url_info_.query_ += "&";
                    }
                    parsed_url_info_.query_ += key;
                    parsed_url_info_.query_ += "=";
                    parsed_url_info_.query_ += val;
                }

                void parse_url_info()
                {
                    parsed_url_info_.parse(url_);
					std::vector<std::string> res;
                    std::stringstream input1(parsed_url_info_.get_query());
                    std::string tmp;
					while (getline(input1, tmp, '&'))
					{
                        std::string key; 
                        std::string val;
                        std::stringstream input2(tmp);
                        getline(input2, key, '=');
                        getline(input2, val, '=');
                        if (key.empty()) {
                            // throw ? 
                        }
                        else {
                            parsed_query_params_[key] = val; 
                        }
					}
                }
                bool has_query_param(const std::string& key)
                {
                    return parsed_query_params_.count(key) > 0; 
                }
                const std::string& get_query_param(const std::string& key)
                {
                    auto iter = parsed_query_params_.find(key);
                    if (iter != parsed_query_params_.end())
                        return iter->second; 
                    else {
                        // throw ??
                        return ""; 
                    }
                }
            private:
                http_method method_ = HTTP_HEAD;
                bool keep_alive_ = false;
                std::string url_;
                http_url_info parsed_url_info_;
                http_version version_;
                std::map<std::string, std::string> parsed_query_params_; 
            };

			class http_response : public http_header_set, public http_body {
			public:
				http_response()
					:status_code_(HTTP_STATUS_OK)
				{}
				http_response(const http_response&) = default;
				http_response& operator=(const http_response&) = default;
				http_response(http_response&&) = default;
				http_response& operator=(http_response&&) = default;

				void reset() {
					http_header_set::reset();
					http_body::reset();
					version_.reset();
					keep_alive_ = false;
					status_code_ = 0; 
					status_text_.clear();
				}

				bool is_keep_alive() const { return keep_alive_; }

				void set_keep_alive(bool keep_alive) { keep_alive_ = keep_alive; }

				const http_version& get_version() const { return version_; }

				void set_version(const http_version& version) { version_ = version; }

				void append_status(const char* data, size_t len) { status_text_.append(data, len); }

				void set_status_text(const std::string& text) { status_text_ = text; }

				const std::string& get_status_text() const { return status_text_;  }


				void set_status_code(uint32_t code) {
					status_code_ = code; 
					if (status_text_.empty()) {
						status_text_ = http_status_str((http_status)status_code_);
					}
				}

				uint32_t get_status_code() const { return status_code_; }

				std::string to_string() const
				{
					std::ostringstream  oss;
					if (!body_.empty()) {
						const_cast<http_response&>(*this).add_header("Content-Length", std::to_string(body_.size()));
					}
					oss << "HTTP/" << version_.to_string() << " " << status_code_ << " "
						<< status_text_ << "\r\n";
					for (const auto& pair : headers_) {
						oss << pair.first << ":" << pair.second << "\r\n";
					}
					oss << "\r\n"; 
					oss << body_; 

					return oss.str();
				}
			private:
				uint32_t       status_code_ = 0;
				std::string    status_text_;
				http_version   version_;
				bool           keep_alive_ = false;
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
					if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK || nparsed > len
						|| (nparsed < len && !p_.upgrade)) {
						/*
						std::ostringstream err_msg;
						err_msg << "HTTP Parse error on character " << total_consumed_length_
							<< ": " << http_errno_name(HTTP_PARSER_ERRNO(&p_));
						throw_parse_error(err_msg.str());
						*/
					}
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
                    request_.parse_url_info();
					if (complete_callback_){
						complete_callback_(request_);
					}
                    return 0;
                }
				const http_request& get_request()const { return request_;  }
            private:
                http_request request_;
				request_complete_callback complete_callback_;
            };


			class http_response_parser : public http_parser_base, public http_header_parser, public spdnet::base::noncopyable {
			public:
				using response_complete_callback = std::function<void(const http_response&)>;
			public:
				http_response_parser()
					: http_parser_base(HTTP_RESPONSE, parser_setting<http_response_parser>::instance().get_setting()), http_header_parser(response_) {}

				void set_parse_complete_callback(response_complete_callback&& cb) {
					complete_callback_ = std::move(cb);
				}
				int on_message_begin() {
					response_.reset();
					http_header_parser::reset();
					return 0;
				}

				int on_url(const char* data, size_t len) {
					(void)data; 
					(void)len;
					assert(false);
					return 0;
				}

				int on_status(const char* data, size_t len) {
					response_.append_status(data, len); 
					return 0;
				}

				int on_body(const char* data, size_t len) {
					response_.append_body(data, len);
					return 0;
				}

				int on_message_complete() {
					response_.set_version(http_version(parser_.http_major, parser_.http_minor));
					response_.set_keep_alive(http_should_keep_alive(&parser_) != 0);
					if (complete_callback_) {
						complete_callback_(response_);
					}
					response_.reset();
					return 0;
				}

			private:
				http_response response_;
				response_complete_callback complete_callback_;
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