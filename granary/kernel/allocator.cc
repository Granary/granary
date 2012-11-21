/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

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


    void *heap_alloc(void *, unsigned long long) {
        return 0;
    }


    void heap_free(void *, void *, unsigned long long) {
        return;
    }
}
