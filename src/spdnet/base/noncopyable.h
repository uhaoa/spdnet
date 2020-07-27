#ifndef SPDNET_BASE_NONCOPYABLE_H_
#define SPDNET_BASE_NONCOPYABLE_H_

namespace spdnet {
    namespace base {
        class noncopyable {
        public:
            noncopyable(const noncopyable &) = delete;

            void operator=(const noncopyable &) = delete;

        protected:
            noncopyable() = default;

            ~noncopyable() = default;
        };
    }
}

#endif // SPDNET_BASE_NONCOPYABLE_H_