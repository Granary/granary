/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * allocator.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_HEAP_H_
#define Granary_HEAP_H_

#ifdef __cplusplus

#   include <new>
#   include <type_traits>

extern "C" {
#endif

/// DynamoRIO-compatible heap allocation functions; these are from globally
/// known allocators.

void *granary_heap_alloc(void *, unsigned long long);
void granary_heap_free(void *, void *, unsigned long);
void *granary_heap_alloc_temp_instr(void);

#ifdef __cplusplus
}

namespace granary {
    namespace detail {

        /// Allocate some executable memory. It is assumed that size is
        /// sufficiently large to allow for both user space and kernel
        /// space allocation, and that the user of this allocator will
        /// handle page alignment, etc.
        void *global_allocate_executable(unsigned long size, int) throw();


        /// Free globally allocated executable memory.
        void global_free_executable(void *addr, unsigned long size) throw();


        /// Allocate some non-executable memory.
        void *global_allocate(unsigned long size) throw();


        /// Free some globally allocated memory.
        void global_free(void *addr, unsigned long) throw();
    }


    namespace detail {

        template <typename T, bool=false>
        struct memory_allocator {
            static T *allocate(const unsigned num) throw() {
                const unsigned size(sizeof(T) * num);
                T *addr(reinterpret_cast<T *>(detail::global_allocate(size)));
                memset(addr, 0, size);
                return addr;
            }

            static void free(T *addr, const unsigned num) throw() {
                const unsigned size(sizeof(T) * num);
                detail::global_free(addr, size);
            }
        };

        template <typename T>
        struct memory_allocator<T, true> {
            static T *allocate(const unsigned num) throw() {
                T *addr(memory_allocator<T,false>::allocate(num));
                new (addr) T;
                return addr;
            }

            static void free(T *addr, const unsigned num) throw() {
                for(unsigned i(0); i < num; ++i) {
                    addr[i].~T();
                }
                memory_allocator<T,false>::free(addr, num);
            }
        };
    }

    template <typename T>
    T *allocate_memory(const unsigned num=1) throw() {
        return detail::memory_allocator<
            T,
            !std::is_trivial<T>::value
        >::allocate(num);
    }


    template <typename T>
    void free_memory(T *addr, const unsigned num=1) throw() {
        detail::memory_allocator<
            T,
            !std::is_trivial<T>::value
        >::free(addr, num);
    }
}

#endif /* __cplusplus */

#endif /* Granary_HEAP_H_ */
