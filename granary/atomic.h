/*
 * atomic.h
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#ifndef GR_ATOMIC_H_
#define GR_ATOMIC_H_

#include <atomic>

namespace granary {

    /// Optionally atomic value.
    template <typename T, bool is_atomic>
    struct opt_atomic {
    private:
        T val;

    public:

        typedef opt_atomic<T, is_atomic> self_type;

        opt_atomic(void) throw()
            : val()
        { }

        opt_atomic(self_type &&val_) throw()
            : val(val_.val)
        { }

        opt_atomic(T &&val_) throw()
            : val(val_)
        { }

        inline bool compare_exchange_strong(T &expected, T desired) throw() {
            if(val == expected) {
                val = desired;
                return true;
            }
            return false;
        }

        inline T exchange(T new_val) throw() {
            T old_val(val);
            val = new_val;
            return old_val;
        }

        inline T &operator=(T &&new_val) throw() {
            val = new_val;
            return val;
        }
    };

    template <typename T>
    struct opt_atomic<T, true> : public std::atomic<T> { };
}

#endif /* GR_ATOMIC_H_ */
