/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/state.h"

#include <cstdlib>
#include <stdint.h>

#define PROT_ALL (~0)

extern "C" {

#   include <sys/mman.h>
#   include <unistd.h>

    /// DynamoRIO-compatible heap allocation

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

    /// (un)protect up to two pages
    static int make_page_executable(void *page_start) {

        return -1 != mprotect(
            page_start,
            granary::PAGE_SIZE,
            PROT_EXEC | PROT_READ | PROT_WRITE
        );
    }
}

#ifndef MAP_ANONYMOUS
#   ifdef MAP_ANON
#       define MAP_ANONYMOUS MAP_ANON
#   else
#       define MAP_ANONYMOUS 0
#   endif
#endif

#ifndef MAP_SHARED
#   define MAP_SHARED 0
#endif

namespace granary { namespace detail {

    void *global_allocate_executable(unsigned long size) throw() {

        // shared map just in case we manage to get user space instrumentation
        // working in a more general case.
        void *allocated(mmap(
            nullptr,
            size + ALIGN_TO(size, PAGE_SIZE),
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_ANONYMOUS | MAP_SHARED,
            -1,
            0));

        if(MAP_FAILED == allocated) {
            granary_break_on_fault();
            granary_fault();
        }

        return allocated;
    }


    void global_free_executable(void *addr, unsigned long size) throw() {
        munmap(addr, size);
    }


    void *global_allocate(unsigned long size) throw() {
        return calloc(1, size);
    }


    void global_free(void *addr) throw() {
        return free(addr);
    }
}}

