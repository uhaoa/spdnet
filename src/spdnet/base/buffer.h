#ifndef SPDNET_BASE_BUFFER_H
#define SPDNET_BASE_BUFFER_H

#include <memory>
#include <cstring>

namespace spdnet {
    namespace base {
        class buffer {
        public:
            static constexpr size_t min_buffer_size = 1024;

            ~buffer() {
                if (data_ != nullptr) {
                    free(data_);
                    data_ = nullptr;
                }
            }

            explicit buffer(size_t buffer_size = min_buffer_size) {
                if (buffer_size < min_buffer_size)
                    buffer_size = min_buffer_size;
                if ((data_ = (char *) malloc(sizeof(char) * buffer_size)) != nullptr)
                    data_size_ = buffer_size;
            }

            void write(const char *data, size_t len) {
                if (get_write_valid_count() >= len) {
                    memcpy(get_write_ptr(), data, len);
                    add_write_pos(len);
                } else {
                    size_t left_len = data_size_ - get_length();
                    if (left_len >= len) {
                        adjust_to_head();
                        write(data, len);
                    } else {
                        size_t need_len = len - left_len;
                        if (need_len > 0) {
                            grow(need_len);
                            write(data, len);
                        }
                    }
                }
            }

            size_t get_length() const {
                return write_pos_ - read_pos_;
            }

            char *get_data_ptr() {
                if (read_pos_ < data_size_)
                    return data_ + read_pos_;

                return nullptr;
            }

            void remove_length(size_t len) {
                if (read_pos_ + len <= data_size_)
                    read_pos_ = read_pos_ + len;
            }

            void swap(buffer &other) {
                std::swap(data_, other.data_);
                std::swap(data_size_, other.data_size_);
                std::swap(write_pos_, other.write_pos_);
                std::swap(read_pos_, other.read_pos_);
            }

            void clear() {
                init();
            }

            void grow(size_t len) {
                size_t n = data_size_ + len;
                char *new_data = new char[n];
                memcpy(new_data, data_, write_pos_);
                data_size_ = n;
                delete[] data_;
                data_ = new_data;
            }

            void adjust_to_head() {
                size_t len;
                if (read_pos_ <= 0)
                    return;
                len = get_length();
                if (len > 0)
                    memmove(data_, data_ + read_pos_, len);
                read_pos_ = 0;
                write_pos_ = len;
            }

            void add_write_pos(size_t value) {
                size_t temp = write_pos_ + value;
                if (temp <= data_size_)
                    write_pos_ = temp;
            }

            size_t get_write_valid_count() const {
                return data_size_ - write_pos_;
            }

            size_t get_capacity() const {
                return data_size_;
            }


            char *get_write_ptr() {
                if (write_pos_ < data_size_)
                    return data_ + write_pos_;
                else
                    return nullptr;
            }

            void set_next(buffer *next) {
                next_ = next;
            }

            buffer *get_next() const {
                return next_;
            }

        private:

            void init() {
                read_pos_ = 0;
                write_pos_ = 0;
            }

            char *data_{nullptr};
            size_t data_size_{0};

            size_t write_pos_{0};
            size_t read_pos_{0};

            buffer *next_{nullptr};
        };

    }
}

#endif //SPDNET_BASE_BUFFER_H
