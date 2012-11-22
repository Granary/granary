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
        return cpu->dr_heap_allocator.allocate_untyped(16, size);
        //return malloc(size);
    }

    void heap_free(void *, void *, unsigned long long) {
        //free(ptr);
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

namespace granary { namespace detail {

    void *global_allocate_executable(unsigned long size) throw() {
        void *allocated(malloc(size));
        uint64_t begin_addr((uint64_t) allocated);
        uint64_t end_addr(begin_addr + size);

        // make each page in this allocation executable
        begin_addr -= (begin_addr % PAGE_SIZE);
        end_addr += ALIGN_TO(end_addr, PAGE_SIZE);
        for(uint64_t addr(begin_addr); addr < end_addr; addr += PAGE_SIZE) {
            make_page_executable((void *) addr);
        }

        return (void *) allocated;
    }


    void *global_allocate(unsigned long size) throw() {
        return malloc(size);
    }
}}

