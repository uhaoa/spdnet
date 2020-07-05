#ifndef SPDNET_BASE_SPIN_LOCK_H
#define SPDNET_BASE_SPIN_LOCK_H

#include <atomic>

namespace spdnet {
    namespace base {

        class SpinLock {
        public:
            SpinLock()
                    : atomic_lock_(false) {
            }

            inline void lock() {
                while (atomic_lock_.exchange(true));
            }

            inline void unlock() {
                atomic_lock_ = false;
            }

            inline bool isLocked() const {
                return atomic_lock_;
            }

        private:
            std::atomic_bool atomic_lock_;
        };

    }
}

#endif //SPDNET_BASE_SPIN_LOCK_H
