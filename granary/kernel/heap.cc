/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

extern "C" {

    void *heap_alloc(void *, unsigned long long) {
        return 0;
    }

    void heap_free(void *, void *, unsigned long long) {
        return;
    }

}

