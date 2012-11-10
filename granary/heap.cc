/*
 * heap.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/heap.h"

extern "C" {

#if GRANARY_IN_KERNEL

    void *heap_alloc(void *, unsigned long long) {
        return 0;
    }

    void heap_free(void *, void *, unsigned long long) {
        return;
    }

#else

#   include <stdlib.h>

    void *heap_alloc(void *, unsigned long long size) {
        return malloc(size);
    }

    void heap_free(void *, void *ptr, unsigned long long) {
        free(ptr);
    }

#endif
}

