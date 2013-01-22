/*
 * spin_lock.h
 *
 *  Created on: 2013-01-20
 *      Author: pag
 */

#ifndef SPIN_LOCK_H_
#define SPIN_LOCK_H_

#include <atomic>

namespace granary { namespace smp {

    /// Simple implementation of a spin lock.
    struct spin_lock {
    private:

        std::atomic<bool> is_locked;

    public:

        ~spin_lock(void) throw() = default;

        spin_lock(const spin_lock &) throw() = delete;
        spin_lock &operator=(const spin_lock &) throw() = delete;

        spin_lock(void) throw()
            : is_locked(ATOMIC_VAR_INIT(false))
        { }

        inline void acquire(void) throw() {
            for(;;) {
                if(is_locked.load(std::memory_order_acquire)) {
                    continue;
                }

                if(!is_locked.exchange(true, std::memory_order_acquire)) {
                    break;
                }
            }
        }

        inline void release(void) throw() {
            is_locked.store(false, std::memory_order_release);
        }
    };

}}

#endif /* SPIN_LOCK_H_ */
