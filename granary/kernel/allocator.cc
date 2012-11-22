/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/state.h"
#include "granary/allocator.h"

namespace granary { namespace detail {

    void *(*__vmalloc_exec)(unsigned long size);
    void *(*__vmalloc)(unsigned long size);


    void *global_allocate_executable(unsigned long size) throw() {
        return __vmalloc_exec(size);
    }


    void *global_allocate(unsigned long size) throw() {
        return __vmalloc(size);
    }

}}

extern "C" {


    void *(**kernel_vmalloc_exec)(unsigned long) = &(granary::detail::__vmalloc_exec);
    void *(**kernel_vmalloc)(unsigned long) = &(granary::detail::__vmalloc);


    void *heap_alloc(void *, unsigned long long size) {
        granary::cpu_state_handle cpu;
        return cpu->dr_heap_allocator.allocate_untyped(16, size);
    }


    void heap_free(void *, void *, unsigned long long) {
        return;
    }
}
