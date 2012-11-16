/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/heap.h"
#include "granary/pp.h"

#define PROT_ALL (~0)

enum {
    PAGE_SIZE = 4096
};

extern "C" {

#   include <stdlib.h>
#   include <sys/mman.h>
#   include <unistd.h>

    /// DynamoRIO-compatible heap allocation

    void *heap_alloc(void *, unsigned long long size) {
        return malloc(size);
    }

    void heap_free(void *, void *ptr, unsigned long long) {
        free(ptr);
    }

    /// (un)protect up to two pages
    static int make_page_executable(void *page_start) {

        return -1 != mprotect(
            page_start,
            PAGE_SIZE,
            PROT_EXEC | PROT_READ | PROT_WRITE
        );
    }
}

#include <stdint.h>

namespace granary {

    void *allocate_executable(cpu_info *, unsigned size) {
        void *allocated(heap_alloc(nullptr, PAGE_SIZE * 2));
        uint64_t addr((uint64_t) allocated);
        addr += ALIGN_TO(addr, PAGE_SIZE);
        make_page_executable((void *) addr);
        return (void *) addr;

        (void) size;
    }
}

