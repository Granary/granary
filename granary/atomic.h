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

    template <typename T, bool is_atomic>
    struct opt_atomic;

    /// Optionally atomic value.
    template <typename T>
    struct opt_atomic<T, false> {
    private:
        T val;

    public:

        typedef opt_atomic<T, false> self_type;

        inline opt_atomic(void) throw()
            : val()
        { }

        inline opt_atomic(self_type &&val_) throw()
            : val(val_.val)
        { }

        inline opt_atomic(T &&val_) throw()
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

        inline T load(void) const throw() {
            return val;
        }

        inline void store(T new_val) throw() {
            val = new_val;
        }
    };

    template <typename T>
    struct opt_atomic<T, true> : public std::atomic<T> { };
}

#endif /* GR_ATOMIC_H_ */
