#ifndef SPDNET_BASE_BUFFER_POOL_H
#define SPDNET_BASE_BUFFER_POOL_H

#include <memory>
#include <cstring>
#include <array>
#include <cassert>
#include <limits>
#include <spdnet/base/buffer.h>
#include <spdnet/base/singleton.h>

namespace spdnet {
    namespace base {
        class buffer_pool : public spdnet::base::noncopyable {
        public:
            static constexpr size_t max_pool_size = 32;

            buffer_pool() {
                for (size_t i = 0; i < max_pool_size; i++) {
                    pool_[i] = nullptr;
                }
            }

            ~buffer_pool() {
                for (size_t i = 0; i < max_pool_size; i++) {
                    buffer *ptr = pool_[i].load(std::memory_order_relaxed);
                    while (ptr) {
                        buffer *next = ptr->get_next();
                        delete ptr;
                        ptr = next;
                    }
                    pool_[i] = nullptr;
                }
            }

            void recycle_buffer(buffer *buf) {
                assert(buf != nullptr);
                uint32_t index = get_index(buf->get_capacity());
                auto &pool_head = pool_[index];
                buffer *ptr = nullptr;
                do {
                    ptr = pool_head.load(std::memory_order_relaxed);
                    buf->set_next(ptr);
                } while (!pool_head.compare_exchange_weak(
                        ptr, buf, std::memory_order_release, std::memory_order_relaxed));
            }

            buffer *alloc_buffer(size_t size) {
                size_t index = get_index(size);
                assert(index < pool_.size());
                auto &pool_head = pool_[index];
                /*
                buffer *buffer = pool_head.load(std::memory_order_acquire);
                if (SPDNET_PREDICT_FALSE(!buffer)) {
                    return new buffer(size);
                }
                */
                buffer *ptr = nullptr;
                do {
                    ptr = pool_head.load(std::memory_order_relaxed);
                } while (ptr && !pool_head.compare_exchange_weak(
                        ptr, ptr->get_next(), std::memory_order_release, std::memory_order_relaxed));
                if (!ptr)
                    ptr = new buffer(size);
                return ptr;
            }

        private:
            static size_t get_index(size_t size) {
                size_t index = 0;
                if (SPDNET_PREDICT_FALSE(size > buffer::min_buffer_size)) {
                    size_t next_size = buffer::min_buffer_size;
                    while (size > next_size) {
                        index++;
                        if ((std::numeric_limits<size_t>::max)() - next_size < next_size)
                            break;
                        next_size *= 2;
                    }
                }
                return index;
            }

        private:
            std::array<std::atomic<buffer *>, max_pool_size> pool_;
        };

    }
}

#endif //SPDNET_BASE_BUFFER_POOL_H
