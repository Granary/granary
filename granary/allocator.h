/*
 * heap.h
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_HEAP_H_
#define Granary_HEAP_H_

#ifdef __cplusplus
#   include <new>
#   include "granary/atomic.h"
#   include "granary/type_traits.h"

extern "C" {
#endif

/// DynamoRIO-compatible heap allocation functions; these are from globally
/// known allocators.

void *heap_alloc(void *, unsigned long long);
void heap_free(void *, void *, unsigned long long);

#ifdef __cplusplus
}

namespace granary {

    namespace detail {

        /// Allocate some executable memory. It is assumed that size is
        /// sufficiently large to allow for both user space and kernel
        /// space allocation, and that the user of this allocator will
        /// handle page alignment, etc.
        void *global_allocate_executable(unsigned long size) throw();


        /// Free globally allocated executable memory.
        void global_free_executable(void *addr, unsigned long size) throw();


        /// Allocate some non-executable memory.
        void *global_allocate(unsigned long size) throw();


        /// Free some globally allocated memory.
        void global_free(void *addr) throw();
    }

}


/// Overload operator new for global heap allocation.
inline void *operator new(size_t size) throw() {
    return granary::detail::global_allocate(size);
}


/// Overload operator new for global heap allocation.
inline void *operator new[](size_t size) throw() {
    return granary::detail::global_allocate(size);
}


/// Overload operator delete for global heap freeing.
inline void operator delete(void *addr) throw() {
    granary::detail::global_free(addr);
}


/// Overload operator delete for global heap freeing.
inline void operator delete[](void *addr) throw() {
    granary::detail::global_free(addr);
}

#endif


#endif /* Granary_HEAP_H_ */
