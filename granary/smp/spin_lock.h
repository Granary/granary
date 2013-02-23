/*
 * spin_lock.h
 *
 *  Created on: 2013-01-20
 *      Author: pag
 */

#ifndef SPIN_LOCK_H_
#define SPIN_LOCK_H_

#include <atomic>

#if GRANARY_IN_KERNEL

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

#else
#   include <pthread.h>

namespace granary { namespace smp {

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

        inline void release(void) throw() {
            pthread_mutex_unlock(&mutex);
        }
    };
}}

#endif

#endif /* SPIN_LOCK_H_ */
