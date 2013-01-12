/*
 * utils.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_UTILS_H_
#define Granary_UTILS_H_

#include "granary/pp.h"

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include "granary/allocator.h"
#   include "granary/type_traits.h"

#   include <stdint.h>
#   include <cstring>
#   include <algorithm>
#   include <atomic>
#   include <new>
#endif

namespace granary {

    template <typename ToT, typename FromT>
    FORCE_INLINE ToT unsafe_cast(const FromT &v) throw()  {
        ToT dest;
        memcpy(&dest, &v, sizeof(ToT));
        return dest;
    }

    template <typename ToT, typename FromT>
    FORCE_INLINE ToT unsafe_cast(FromT *v) throw() {
        return unsafe_cast<ToT>(
            reinterpret_cast<uintptr_t>(v)
        );
    }


    namespace detail {
        template <typename T, unsigned extra>
        struct cache_aligned_impl {
            T val;
            unsigned char padding[CACHE_LINE_SIZE - extra];
        } __attribute__((packed));


        /// No padding needed.
        template <typename T>
        struct cache_aligned_impl<T, 0> {
            T val;
        };
    }


    /// Repesents a cache aligned type.
    template <typename T>
    struct cache_aligned
        : public detail::cache_aligned_impl<T, sizeof(T) % CACHE_LINE_SIZE>
    { } __attribute__((aligned (CONFIG_MIN_CACHE_LINE_SIZE)));


    /// Returns an offset of some application code from the beginning of
    /// application code.
    int32_t to_application_offset(uint64_t addr) throw();


    /// Converts an offset from the beginning of application code into an
    /// application code pointer (represented as a uint64_t)
    uint64_t from_application_offset(int32_t) throw();


    /// Represents a simple boxed array type. This is mostly means to more
    /// easily pass transiently-allocated arrays around as arguments/return
    /// values.
    template <typename T>
    struct array {
    private:

        T *elms;
        unsigned num_elms;

    public:

        array(void) throw()
            : elms(nullptr)
            , num_elms(0U)
        { }

        array(T *elms_, unsigned num_elms_) throw()
            : elms(elms_)
            , num_elms(num_elms_)
        { }

        array(array<T> &&that) throw()
            : elms(that.elms)
            , num_elms(that.num_elms)
        { }

        array<T> &operator=(array<T> &&that) throw() {
            elms = that.elms;
            num_elms = that.num_elms;
            return *this;
        }

        inline operator T *(void) throw() {
            return elms;
        }

        inline T &operator[](unsigned i) throw() {
            return elms[i];
        }

        inline const T &operator[](unsigned i) const throw() {
            return elms[i];
        }

        inline T *begin(void) throw() {
            return elms;
        }

        inline T *end(void) throw() {
            return elms + num_elms;
        }

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
        inline void sort(void) throw() {
            std::sort(begin(), end());
        }
#endif
    };


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB

    /// Simple implementation of a spin lock.
    struct spin_lock {
    private:
        std::atomic<bool> lock;

    public:

        spin_lock(void) throw() = default;
        ~spin_lock(void) throw() = default;

        spin_lock(const spin_lock &) throw() = delete;
        spin_lock &operator=(const spin_lock &) throw() = delete;

        inline void acquire(void) throw() {
            while(lock.exchange(true)) { }
        }

        inline void release(void) throw() {
            lock = false;
        }
    };

    /// Simple implementation of a reference counter.
    struct reference_counter {
    private:
        std::atomic<unsigned> counter;
    public:

        reference_counter(void) throw() = default;
        ~reference_counter(void) throw() = default;

        reference_counter(const reference_counter &) throw() = delete;
        reference_counter &operator=(const reference_counter &) throw() = delete;

        /// Increments the reference counter. Returns true if the
        /// reference counter was valid and false otherwise.
        inline bool increment(void) throw() {
            const unsigned prev_val(counter.fetch_add(2, std::memory_order_relaxed));
            return 0U == (prev_val & 1U);
        }

        /// Decrements the reference counter.
        inline void decrement(void) throw() {
            counter.fetch_sub(2, std::memory_order_release);
        }

        /// Reset the reference counter.
        inline void reset(void) throw() {
            counter = 0;
        }

        /// Wait for the reference counter to stabilize.
        inline void wait(void) throw() {
            unsigned expected(0U);
            while(!counter.compare_exchange_weak(
                expected, 1, std::memory_order_seq_cst)) { }
        }
    };


    /// Represents a type where the first type ends with a value
    /// of the second type, and the second type "spills" over to
    /// form a very large array. The assumed use of this type is
    /// that type `T` ends with an array of length 1 of element
    /// of type `V`.
    template <typename T, typename V>
    T *new_trailing_vla(unsigned array_size) throw() {
        const size_t needed_space(sizeof(T) + (array_size - 1) * sizeof(V));
        char *internal(reinterpret_cast<char *>(
            detail::global_allocate(needed_space)));

        if(!std::is_trivial<T>::value) {
            new (internal) T;
        }

        if(!std::is_trivial<V>::value) {
            char *arr_ptr(internal + sizeof(T));
            for(unsigned i(1); i < array_size; ++i) {
                new (arr_ptr) V;
                arr_ptr += sizeof(V);
            }
        }

        return unsafe_cast<T *>(internal);
    }
#endif
}


#endif /* Granary_UTILS_H_ */
