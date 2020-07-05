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
        class BufferPool : public spdnet::base::Singleton<BufferPool> {
        public:
            static constexpr size_t max_pool_size = 32;

            BufferPool() {
                for (size_t i = 0; i < max_pool_size; i++) {
                    pool_[i] = nullptr;
                }
            }

            ~BufferPool() {
                for (size_t i = 0; i < max_pool_size; i++) {
                    Buffer *ptr = pool_[i].load(std::memory_order_relaxed);
                    while (ptr) {
                        Buffer *next = ptr->getNext();
                        delete ptr;
                        ptr = next;
                    }
                    pool_[i] = nullptr;
                }
            }

            void recycleBuffer(Buffer *buffer) {
                assert(buffer != nullptr);
                uint32_t index = getIndex(buffer->getCapacity());
                auto &pool_head = pool_[index];
                Buffer *ptr = nullptr;
                do {
                    ptr = pool_head.load(std::memory_order_relaxed);
                    buffer->setNext(ptr);
                } while (!pool_head.compare_exchange_weak(
                        ptr, buffer, std::memory_order_release, std::memory_order_relaxed));
            }

            Buffer *allocBufferBySize(size_t size) {
                size_t index = getIndex(size);
                assert(index < pool_.size());
                auto &pool_head = pool_[index];
                Buffer *buffer = pool_head.load(std::memory_order_acquire);
                if (SPDNET_PREDICT_FALSE(!buffer)) {
                    return new Buffer(size);
                }
                Buffer *ptr = nullptr;
                do {
                    ptr = pool_head.load(std::memory_order_relaxed);
                } while (!pool_head.compare_exchange_weak(
                        ptr, ptr->getNext(), std::memory_order_release, std::memory_order_relaxed));

                return ptr;
            }

        private:
            static size_t getIndex(size_t size) {
                size_t index = 0;
                if (SPDNET_PREDICT_FALSE(size > Buffer::min_buffer_size)) {
                    size_t next_size = Buffer::min_buffer_size;
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
            std::array<std::atomic<Buffer *>, max_pool_size> pool_;
        };

    }
}

#endif //SPDNET_BASE_BUFFER_POOL_H
