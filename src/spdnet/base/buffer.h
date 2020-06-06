#ifndef SPDNET_BASE_BUFFER_H
#define SPDNET_BASE_BUFFER_H

#include <memory>
#include <cstring>

namespace spdnet::base {
    class Buffer {
    public:
        ~Buffer() {
            if (data_ != nullptr) {
                free(data_);
                data_ = nullptr;
            }
        }

        explicit Buffer(size_t buffer_size = 1024) {
            if ((data_ = (char *) malloc(sizeof(char) * buffer_size)) != nullptr)
                data_size_ = buffer_size;
        }

        void write(const char *data, size_t len) {
            if (getWriteValidCount() >= len) {
                memcpy(getWritePtr(), data, len);
                addWritePos(len);
            } else {
                size_t left_len = data_size_ - getLength();
                if (left_len >= len) {
                    adjustToHead();
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

        size_t getLength() const {
            return write_pos_ - read_pos_;
        }

        char *getDataPtr() {
            if (read_pos_ < data_size_)
                return data_ + read_pos_;

            return nullptr;
        }

        void removeLength(size_t len) {
            if (read_pos_ + len <= data_size_)
                read_pos_ = read_pos_ + len;
        }

        void swap(Buffer &other) {
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

        void adjustToHead() {
            size_t len;
            if (read_pos_ <= 0)
                return;
            len = getLength();
            if (len > 0)
                memmove(data_, data_ + read_pos_, len);
            read_pos_ = 0;
            write_pos_ = len;
        }

        void addWritePos(size_t value) {
            size_t temp = write_pos_ + value;
            if (temp <= data_size_)
                write_pos_ = temp;
        }

        size_t getWriteValidCount() const {
            return data_size_ - write_pos_;
        }

        size_t getCapacity() const {
            return data_size_;
        }


        char *getWritePtr() {
            if (write_pos_ < data_size_)
                return data_ + write_pos_;
            else
                return nullptr;
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
    };

}

#endif //SPDNET_BASE_BUFFER_H
