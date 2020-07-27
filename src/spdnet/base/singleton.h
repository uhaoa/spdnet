#ifndef SPDNET_BASE_SINGLETON_H_
#define SPDNET_BASE_SINGLETON_H_

#include <spdnet/base/noncopyable.h>

namespace spdnet {
    namespace base {
        template<typename T>
        class singleton : public spdnet::base::noncopyable {
        public:
            static T &instance() {
                std::call_once(once_, &singleton<T>::init);
                return *instance_;
            }

        private:
            static void init() {
                instance_ = new T;
            }

        private:
            static std::once_flag once_;
            static T *instance_;
        };

        template<typename T>
        T *singleton<T>::instance_ = nullptr;

        template<typename T>
        std::once_flag singleton<T>::once_;
    }
}

#endif // SPDNET_BASE_SINGLETON_H_