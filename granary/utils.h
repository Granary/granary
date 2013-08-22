/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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
#   include <stdint.h>
#   include <cstring>
#   include "granary/allocator.h"
#   include "granary/type_traits.h"
#   include <atomic>
#   include <new>
#endif

namespace granary {


    /// Non-integral, non-pointer type to something else.
    ///
    /// Note: `__builtin_memcpy` is used instead of `memcpy`, mostly for the
    ///       sake of kernel code where it sometimes seems that the optimisation
    ///       of inlining a normal `memcpy` is not done.
    template <
        typename ToT,
        typename FromT,
        typename std::enable_if<
            !std::is_pointer<FromT>::value,
            int
        >::type = 0
    >
    FORCE_INLINE ToT unsafe_cast(FromT v) throw() {
        static_assert(sizeof(FromT) == sizeof(ToT),
            "Dangerous unsafe cast between two types of different sizes.");

        ToT dest;
        __builtin_memcpy(&dest, &v, sizeof(ToT));
        return dest;
    }


    /// Pointer to non-pointer integral type.
    template <
        typename ToT,
        typename FromT,
        typename std::enable_if<
            std::is_pointer<FromT>::value
                && std::is_integral<ToT>::value
                && !std::is_pointer<ToT>::value,
            int
        >::type = 0
    >
    FORCE_INLINE ToT unsafe_cast(FromT v) throw() {
        return static_cast<ToT>(reinterpret_cast<uintptr_t>(v));
    }


    /// Pointer to pointer type.
    template <
        typename ToT,
        typename FromT,
        typename std::enable_if<
            std::is_pointer<FromT>::value
                && std::is_pointer<ToT>::value,
            int
        >::type = 0
    >
    FORCE_INLINE ToT unsafe_cast(FromT v) throw() {
        return reinterpret_cast<ToT>(reinterpret_cast<uintptr_t>(v));
    }


    /// Pointer to non-integral, non-pointer type.
    template <
        typename ToT,
        typename FromT,
        typename std::enable_if<
        std::is_pointer<FromT>::value
            && !std::is_integral<ToT>::value
            && !std::is_pointer<ToT>::value,
        int
        >::type = 0
    >
    FORCE_INLINE ToT unsafe_cast(FromT v) throw() {
        return unsafe_cast<ToT>(reinterpret_cast<uintptr_t>(v));
    }


#if GRANARY_IN_KERNEL
    FORCE_INLINE static bool is_valid_address(uintptr_t addr) throw() {
        return 0 != (addr & 0x0000800000000000ULL); // && (addr >> 48) != 0xdead;
    }


    template <typename T>
    FORCE_INLINE static bool is_valid_address(T *addr) throw() {
        return is_valid_address(reinterpret_cast<uintptr_t>(addr));
    }
#else
    FORCE_INLINE static bool is_valid_address(uintptr_t addr) throw() {
        return 4095UL < addr;
    }


    template <typename T>
    FORCE_INLINE static bool is_valid_address(T *addr) throw() {
        return is_valid_address(reinterpret_cast<uintptr_t>(addr));
    }
#endif /* GRANARY_IN_KERNEL */


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


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
    /// Represents some statically allocated data, with delayed construction.
    /// This exists to prevent static (Granary) destructors from being
    /// instrumented in user space.
    template <typename T>
    struct static_data {
        char memory[sizeof(T)];
        T *self;

        template <typename... Args>
        void construct(Args... args) throw() {
            self = new (&(memory[0])) T(args...);
        }

        inline T *operator->(void) throw() {
            return self;
        }
    } __attribute__((aligned (16)));


    /// Represents a fixed-size set of elements, where each entry in the set is
    /// uniquely identified by an ID. The type of element in the set (`KV`) must
    /// be convertible to a `uintptr_t`.
    template <unsigned max_items, typename KV>
    struct atomic_id_set {
    private:
        std::atomic<KV> set[max_items];

        KV default_value;
        KV missing_value;

    public:
        atomic_id_set(KV default_value_, KV missing_value_) throw()
            : default_value(default_value_)
            , missing_value(missing_value_)
        { }

        /// Add an entry to the set, and return its ID.
        unsigned add(KV value) throw() {

            const uintptr_t value_bits(
                granary::unsafe_cast<uintptr_t>(value));

            uintptr_t index;
            uintptr_t next_index(value_bits % max_items);

            KV found_value(default_value);

            unsigned last_missing_index(0);
            bool found_missing_slot(false);

            do {
                index = next_index;
                next_index = (index + 1) % max_items;
                found_value = set[index].load(std::memory_order_relaxed);

                if(missing_value == found_value) {
                    last_missing_index = index;
                    found_missing_slot = true;
                } else if(default_value == found_value) {

                    bool updated(false);

                    // If we previously walked across an entry that can be
                    // re-allocated, then re-allocate it.
                    if(found_missing_slot) {
                        updated = set[last_missing_index].compare_exchange_weak(
                            missing_value,
                            value);
                    }

                    // If we didn't re-allocate a previously allocated
                    if(!updated) {
                        updated = set[index].compare_exchange_weak(
                            found_value,
                            value);
                    }

                    if(updated) {
                        found_value = value;
                    }
                }
            } while(found_value != value);

            return index;
        }

        /// Get an element from the set given its ID.
        KV get(unsigned id) throw() {
            KV val(set[id % max_items].load());
            if(missing_value == val) {
                return default_value;
            }
            return val;
        }

        /// Remove the element in the set with ID `id`.
        void remove(unsigned id) throw() {
            set[id % max_items].store(missing_value);
        }
    };


    /// Represents a bitset.
    template <unsigned num_bits>
    struct bitset {

        typedef uint64_t elem_type;

        enum {
            MIN_NUM_BITS = sizeof(elem_type) * 8,
            TRAILING_BITS = num_bits % MIN_NUM_BITS,
            MISSING_BITS = MIN_NUM_BITS - TRAILING_BITS,
            NUM_BITS = TRAILING_BITS ? (num_bits + MISSING_BITS) : num_bits,
            NUM_ELEMS = NUM_BITS / MIN_NUM_BITS
        };

        elem_type elems[NUM_ELEMS];

        void set(unsigned offset, bool value) throw() {
            const elem_type one(1);
            const elem_type mask(1 << (offset % MIN_NUM_BITS));
            if(value) {
               elems[offset / MIN_NUM_BITS] |= mask;
            } else {
                elems[offset / MIN_NUM_BITS] &= ~mask;
            }
        }

        bool get(unsigned offset) const throw() {
            const elem_type one(1);
            const elem_type mask(1 << (offset % MIN_NUM_BITS));
            return 0 != elems[offset / MIN_NUM_BITS];
        }
    };
#endif


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
    };


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
    template <typename T, typename V>
    size_t sizeof_trailing_vla(unsigned array_size) throw() {
        return sizeof(T) + (array_size - 1) * sizeof(V);
    }


    /// Represents a type where the first type ends with a value
    /// of the second type, and the second type "spills" over to
    /// form a very large array. The assumed use of this type is
    /// that type `T` ends with an array of length 1 of element
    /// of type `V`.
    template <typename T, typename V>
    T *new_trailing_vla(unsigned array_size) throw() {
        const size_t needed_space(sizeof_trailing_vla<T, V>(array_size));
        char *internal(reinterpret_cast<char *>(
            detail::global_allocate(needed_space)));

        memset(internal, 0, needed_space);

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


    /// Frees a trailing VLA.
    template <typename T, typename V>
    void free_trailing_vla(T *vla, unsigned array_size) throw() {
        detail::global_free(vla, sizeof_trailing_vla<T, V>(array_size));
    }
#endif


    namespace {
        template<typename T>
        class type_id_impl {
        public:
            static unsigned get(void) throw() {
                static unsigned id_(0);
                return id_++;
            }
        };
    }


    /// Used to statically define IDs for particular classes.
    template <typename T, typename Category=void>
    class type_id {
    public:
        static unsigned get(void) throw() {
            static type_id id_(type_id_impl<Category>::get());
            return id_;
        }
    };


    /// Represents a way of indirectly calling a function and being able to
    /// save--or pretend to save--its return value (if any) for later returning
    /// in a type-safe way.
    template <typename R, typename... Args>
    struct function_call {
    private:
        R (*func)(Args...);
        R ret_value;

    public:

        inline function_call(R (*func_)(Args...)) throw()
            : func(func_)
        { }

        inline void operator()(Args... args) throw() {
            ret_value = func(args...);
        }

        inline R yield(void) throw() {
            return ret_value;
        }
    };


    /// Read-critical section returning nothing.
    template <typename... Args>
    struct function_call<void, Args...> {
    private:
        void (*func)(Args...);

    public:

        inline function_call(void (*func_)(Args...)) throw()
            : func(func_)
        { }

        inline void operator()(Args... args) throw() {
            func(args...);
        }

        inline void yield(void) throw() {
            return;
        }
    };


    /// Apply an operator over the arguments of an argument pack.
    template <typename Operator, typename... Types>
    struct apply_to_values;


    template <typename Operator, typename T0, typename... Types>
    struct apply_to_values<Operator, T0, Types...> {
    public:
        FORCE_INLINE
        static void apply(T0 &value, Types&... values) throw() {
            Operator::template apply<T0>(value);
            apply_to_values<Operator, Types...>::apply(values...);
        }
    };

    template <typename Operator, typename T0>
    struct apply_to_values<Operator, T0> {
    public:
        FORCE_INLINE
        static void apply(T0 &value) throw() {
            Operator::template apply<T0>(value);
        }
    };


    template <typename Operator>
    struct apply_to_values<Operator> {
    public:
        FORCE_INLINE
        static void apply(void) throw() { }
    };
}


#endif /* Granary_UTILS_H_ */
