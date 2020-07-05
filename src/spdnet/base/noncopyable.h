#ifndef SPDNET_BASE_NONCOPYABLE_H_
#define SPDNET_BASE_NONCOPYABLE_H_

namespace spdnet {
    namespace base {
        class NonCopyable {
        public:
            NonCopyable(const NonCopyable &) = delete;

            void operator=(const NonCopyable &) = delete;

        protected:
            NonCopyable() = default;

            ~NonCopyable() = default;
        };
    }
}

#endif // SPDNET_BASE_NONCOPYABLE_H_