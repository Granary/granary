/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * heap.h
 *
 *  Created on: 2013-10-28
 *      Author: Peter Goodman
 */

#ifndef GRANARY_GENERIC_HEAP_H_
#define GRANARY_GENERIC_HEAP_H_

#include <atomic>

namespace granary {

    enum {
        HEAP_MIN_SCALE = 3,
        HEAP_MIN_OBJECT_SIZE = (1 << HEAP_MIN_SCALE),
        HEAP_NUM_FREE_LISTS = 64 - HEAP_MIN_SCALE,
        HEAP_MAX_ADJUSTED_SCALE = HEAP_NUM_FREE_LISTS - HEAP_MIN_SCALE
    };


    struct heap_block {
        uint8_t *min;
        uint8_t *max;
        std::atomic<unsigned> curr;
        heap_block *next;
    };


    struct free_object {
        free_object *next;
    };


    struct free_object_list {
        granary::smp::atomic_spin_lock lock;
        std::atomic<free_object *> head;
    };


    struct heap {
        unsigned block_size;

        enum {
            HEAP_EXECUTABLE,
            HEAP_DATA
        } heap_kind;

        enum {
            HEAP_FIXED,
            HEAP_GROWING
        } heap_grow_constraint;

        heap_block *block;

        free_object_list free_lists[HEAP_NUM_FREE_LISTS];
    };

    // TODO

}

#endif /* GRANARY_GENERIC_HEAP_H_ */
