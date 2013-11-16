/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * spin_lock.h
 *
 *  Created on: 2013-01-20
 *      Author: pag
 */

#ifndef SPIN_LOCK_H_
#define SPIN_LOCK_H_

#include <atomic>

namespace granary {

    /// Simple implementation of a spin lock.
    struct atomic_spin_lock {
    private:

        std::atomic<bool> is_locked;

    public:

        ~atomic_spin_lock(void) = default;

        atomic_spin_lock(const atomic_spin_lock &) throw() = delete;
        atomic_spin_lock &operator=(const atomic_spin_lock &) throw() = delete;

        atomic_spin_lock(void) throw()
            : is_locked(ATOMIC_VAR_INIT(false))
        { }

        /// Acquire the spin lock.
        ///
        /// TODO: Probably want things so that the person acquiring a spin
        ///       lock needs to disable interrupts.
        inline void acquire(void) throw() {
            IF_KERNEL( eflags flags = granary_disable_interrupts(); )

            for(;;) {
                if(is_locked.load(std::memory_order_acquire)) {
                    ASM("pause;");
                    continue;
                }

                if(!is_locked.exchange(true, std::memory_order_acquire)) {
                    break;
                }

                ASM("pause;");
            }

            IF_KERNEL( granary_store_flags(flags); )
        }


        /// Try to acquire the spin lock.
        inline bool try_acquire(void) throw() {
            if(is_locked.load(std::memory_order_relaxed)) {
                return false;
            }

            IF_KERNEL( eflags flags = granary_disable_interrupts(); )
            const bool acquired(
                !is_locked.exchange(true, std::memory_order_seq_cst));
            IF_KERNEL( granary_store_flags(flags); )

            return acquired;
        }

        inline void release(void) throw() {
            is_locked.store(false, std::memory_order_release);
        }
    };

}

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   if CONFIG_ENV_KERNEL

namespace granary {
    typedef atomic_spin_lock spin_lock;
}

#   else
#       include <pthread.h>

namespace granary {

    /// Simple implementation of lock using pthread mutexes.
    struct spin_lock {
    private:

        pthread_mutex_t mutex;

    public:

        ~spin_lock(void) throw() {
            pthread_mutex_destroy(&mutex);
        }

        spin_lock(const spin_lock &) throw() = delete;
        spin_lock &operator=(const spin_lock &) throw() = delete;

        spin_lock(void) throw() {
            pthread_mutex_init(&mutex, nullptr);
        }

        inline void acquire(void) throw() {
            pthread_mutex_lock(&mutex);
        }

        inline bool try_acquire(void) throw() {
            return 0 == pthread_mutex_trylock(&mutex);
        }

        inline void release(void) throw() {
            pthread_mutex_unlock(&mutex);
        }
    };
}

#   endif /* CONFIG_ENV_KERNEL */
#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */

#endif /* SPIN_LOCK_H_ */
