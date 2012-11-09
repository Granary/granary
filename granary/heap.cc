/*
 * heap.cc
 *
 *  Created on: 2012-11-09
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/heap.h"

extern "C" {

    void *heap_alloc(void *, unsigned long long) {
        return 0;
    }

    void heap_free(void *, void *, unsigned long long) {
        return;
    }
}

