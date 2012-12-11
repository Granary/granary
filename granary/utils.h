/*
 * utils.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_UTILS_H_
#define Granary_UTILS_H_

#include "pp.h"

#include <stdint.h>
#include <cstring>
#include <algorithm>

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



    namespace detail {

    }


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

        inline void sort(void) throw() {
            std::sort(begin(), end());
        }
    };
}


#endif /* Granary_UTILS_H_ */
