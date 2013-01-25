/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/state.h"
#include "granary/allocator.h"

namespace granary { namespace detail {

    void *(*__vmalloc_exec)(unsigned long);
    void *(*__vmalloc)(unsigned long);
    void (*__vfree)(void *);

    void *global_allocate_executable(unsigned long size) throw() {
        return __vmalloc_exec(size);
    }


    void global_free_executable(void *addr, unsigned long) throw() {
        return __vfree(addr);
    }


    void *global_allocate(unsigned long size) throw() {
        void * mem(__vmalloc(size));
        memset(mem, 0, size);
        return mem;
    }

    void global_free(void *addr) throw() {
        return __vfree(addr);
    }

}}

extern "C" {


    void *(**kernel_vmalloc_exec)(unsigned long) = &(granary::detail::__vmalloc_exec);
    void *(**kernel_vmalloc)(unsigned long) = &(granary::detail::__vmalloc);
    void (**kernel_vfree)(void *) = &(granary::detail::__vfree);

    void *heap_alloc(void *, unsigned long long size) {
        granary::cpu_state_handle cpu;
        return cpu->transient_allocator.allocate_untyped(16, size);
    }


    void heap_free(void *, void *addr, unsigned long long) {
#if CONFIG_PRECISE_ALLOCATE
        granary::detail::global_free(addr);
#endif
        (void) addr;
    }
}
