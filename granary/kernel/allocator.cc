/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * heap.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <atomic>

#include "granary/state.h"
#include "granary/allocator.h"
#include "granary/globals.h"
#include "granary/spin_lock.h"


#define ENABLE_SPLITTING 1


extern "C" {
    void *kernel_alloc_executable(unsigned long, int);
}


namespace granary { namespace detail {

    void *global_allocate_executable(unsigned long size, int where) throw() {
        return kernel_alloc_executable(size, where);
    }


    void global_free_executable(void *, unsigned long) throw() {
        // TODO: memory leak if Granary is unloaded.
    }


    enum {
        _1_MB = 1048576,
        HEAP_SIZE = _1_MB * 70,
        MIN_SCALE = 3,
        UNSIGNED_LONG_NUM_BITS = sizeof(unsigned long) * 8,
        MIN_OBJECT_SIZE = (1 << MIN_SCALE),
        NUM_FREE_LISTS = 64 - MIN_SCALE,
        MAX_ADJUSTED_SCALE = NUM_FREE_LISTS - MIN_SCALE
    };


    /// Free object descriptor.
    struct free_object {
        free_object *next;
    };


    /// Free list.
    struct free_list {
        granary::atomic_spin_lock lock;
        std::atomic<free_object *> head;
    };


    /// Heap memory for Granary in kernel space.
    static uint8_t HEAP[HEAP_SIZE];


    /// Bump pointer.
    static std::atomic<unsigned> HEAP_INDEX(ATOMIC_VAR_INIT(0));


    /// Free lists, based on size.
    static free_list FREE_LISTS[NUM_FREE_LISTS];


    /// Returns the log base 2 of a number.
    static inline uint32_t log_base_2(unsigned long x) throw() {
        return ((UNSIGNED_LONG_NUM_BITS - 1) - __builtin_clzl(x));
    }


    /// Return the smallest power of two value greater than x.
    static inline unsigned long next_power_of_2(unsigned long x) throw() {
        return 1UL << (UNSIGNED_LONG_NUM_BITS - __builtin_clzl(x - 1));
    }


    /// Round the size of some data.
    __attribute__((hot))
    static unsigned long allocation_size(unsigned long size) throw() {
        if(MIN_OBJECT_SIZE > size) {
            return MIN_OBJECT_SIZE;
        }

        const unsigned long high_power(next_power_of_2(size));
        const unsigned long low_power(high_power / 2);

        return (low_power >= size) ? low_power : high_power;
    }


    /// Returns true of an address is a head address.
    __attribute__((hot))
    inline static bool is_heap_address(void *ptr_) throw() {
        const uint8_t *ptr(reinterpret_cast<uint8_t *>(ptr_));
        return ptr >= &(HEAP[0]) && ptr < &(HEAP[HEAP_SIZE]);
    }


#if ENABLE_SPLITTING
    /// Try to split a large heap object into a smaller heap object.
    __attribute__((hot))
    static free_object *split_large_object(unsigned scale) throw() {
        const unsigned next_scale(scale + 1);

        if(next_scale >= MAX_ADJUSTED_SCALE) {
            return nullptr;
        }

        // Go to the next largest.
        FREE_LISTS[next_scale].lock.acquire();
        free_object *object(nullptr);
        free_object *next_object(nullptr);
        object = FREE_LISTS[next_scale].head.load();
        if(!object) {
            FREE_LISTS[next_scale].lock.release();

            // Try to recursively split a larger object.
            object = split_large_object(next_scale);
            if(!object) {
                return nullptr;
            }

            FREE_LISTS[next_scale].lock.acquire();
            next_object = FREE_LISTS[next_scale].head.load();
        } else {
            next_object = object->next;
        }
        FREE_LISTS[next_scale].head.store(next_object);
        FREE_LISTS[next_scale].lock.release();

        const unsigned object_size(allocation_size(
            1 << (scale + MIN_SCALE)));

        global_free(
            &(unsafe_cast<uint8_t *>(object)[object_size / 2]),
            object_size / 2
        );

        return object;
    }
#endif


    /// Allocate some data from the heap.
    __attribute__((hot))
    void *global_allocate(unsigned long size_) throw() {

        ASSERT(0 < size_);

        const unsigned long size(allocation_size(size_));
        const unsigned scale(log_base_2(size) - MIN_SCALE);

        ASSERT(scale <= NUM_FREE_LISTS);
        ASSERT(size >= size_);

        for(;;) {
            if(!FREE_LISTS[scale].head.load()) {

                const unsigned curr_heap_index(HEAP_INDEX.fetch_add(size));
                const unsigned long next_heap_index(curr_heap_index + size);

#if ENABLE_SPLITTING

                // Try to detect if our chosen heap size is too small.
                if(next_heap_index > HEAP_SIZE) {
                    free_object *object(split_large_object(scale));
                    if(!object) {
                        ASSERT(false);
                    }
                    continue;
                }
#else
                FAULT_IF(next_heap_index >= HEAP_SIZE);
#endif

                return &(HEAP[curr_heap_index]);
            } else {
                free_object *object(nullptr);
                FREE_LISTS[scale].lock.acquire();
                object = FREE_LISTS[scale].head.load();

                // Deal with the race condition where two threads/cores try
                // to allocate objects of the same size from the same free list.
                if(!object) {
                    FREE_LISTS[scale].lock.release();
                    continue;
                }

                FREE_LISTS[scale].head.store(object->next);
                FREE_LISTS[scale].lock.release();

                return object;
            }
        };
    }


    /// Free some memory back to the heap.
    __attribute__((hot))
    void global_free(void *addr, unsigned long size_) throw() {

        if(!is_heap_address(addr)) {
            return;
        }

        const unsigned long size(allocation_size(size_));
        const unsigned scale(log_base_2(size) - MIN_SCALE);

        // Add it to the free list.
        FREE_LISTS[scale].lock.acquire();
        free_object *next_free = FREE_LISTS[scale].head.load();
        free_object *new_free(unsafe_cast<free_object *>(addr));
        new_free->next = next_free;
        FREE_LISTS[scale].head.store(new_free);
        FREE_LISTS[scale].lock.release();
    }
}}

extern "C" {

    void *granary_heap_alloc(void *, unsigned long long size) {
        granary::cpu_state_handle cpu;
        return cpu->transient_allocator.allocate_untyped(16, size);
    }


    void granary_heap_free(void *, void *, unsigned long) { }


    /// Return temporarily allocated space for an instruction.
    void *granary_heap_alloc_temp_instr(void) {
        granary::cpu_state_handle cpu;
        return cpu->instruction_allocator.allocate_array<uint8_t>(32);
    }
}
