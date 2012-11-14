/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

extern "C" {

#   include <stdlib.h>

    void *heap_alloc(void *, unsigned long long size) {
        return malloc(size);
    }

    void heap_free(void *, void *ptr, unsigned long long) {
        free(ptr);
    }

}

