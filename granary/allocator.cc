/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * allocator.cc
 *
 *  Created on: 2013-11-13
 *      Author: Peter Goodman
 */

#include "granary/globals.h" // for app_pc
#include "granary/state.h"  // for detail::fragment_allocator::SLAB_SIZE
#include "granary/detach.h" // for GRANARY_DETACH_POINT_ERROR

#if CONFIG_ENV_KERNEL
extern "C" {
    /// Mark a range of memory as being executable.
    extern void kernel_make_pages_executable(void *begin, void *end);
}
#else
#   include <sys/mman.h>
#   include <unistd.h>
#   define PROT_ALL (~0)
#   ifndef MAP_ANONYMOUS
#       ifdef MAP_ANON
#           define MAP_ANONYMOUS MAP_ANON
#       else
#           define MAP_ANONYMOUS 0
#       endif
#   endif
#   ifndef MAP_SHARED
#       define MAP_SHARED 0
#   endif
#endif


/// Exposed to the kernel in `module.c` for `is_host_address`.
extern "C" {
    uintptr_t GRANARY_EXEC_START = 0;
    uintptr_t GRANARY_EXEC_END = 0;
}


namespace granary { namespace detail {


    enum executable_memory_kind {
        EXEC_CODE_CACHE = 0,
        EXEC_GEN_CODE = 1,
        EXEC_WRAPPER = 2
    };


    enum {
        _1_MB = 1048576,
        CODE_CACHE_SIZE = 100 * _1_MB,

        // Maximum size of the part of the code cache containing basic blocks /
        // fragments.
        FRAGMENT_CACHE_MAX_SIZE = CODE_CACHE_SIZE - _1_MB,

        // Keep this consistent with `granary/state.h`,
        // fragment_allocator_config::SLAB_SIZE
        FRAGMENT_SLAB_SIZE = fragment_allocator_config::SLAB_SIZE,

        // Maximum number of fragment slabs.
        MAX_NUM_FRAGMENT_SLABS = FRAGMENT_CACHE_MAX_SIZE / FRAGMENT_SLAB_SIZE
    };


    struct page {
        uint8_t data[CONFIG_ARCH_PAGE_SIZE];
    } __attribute__((aligned (CONFIG_ARCH_PAGE_SIZE)));


    enum {
        NUM_EXEC_PAGES = CODE_CACHE_SIZE / sizeof(page)
    };


    static page EXECUTABLE_AREA[NUM_EXEC_PAGES] = {{{0xCC}}};


    /// Layout of the executable region:
    ///
    ///  GRANARY_EXEC_START          GEN_CODE_START   WRAPPER_START  GRANARY_EXEC_END
    ///      |--------------->              <--------------|-------->      |
    ///                CODE_CACHE_END                          WRAPPER_END
    ///
    uintptr_t CODE_CACHE_END = 0;
    static uintptr_t GEN_CODE_START = 0;
    static uintptr_t WRAPPER_START = 0;
    static uintptr_t WRAPPER_END = 0;


    static void *FRAGMENT_SLABS[MAX_NUM_FRAGMENT_SLABS] = {NULL};


    /// Given an arbitrary address into a basic block, we want to be able to find
    /// the associated basic block info / meta-data. The approach is to first find
    /// the slab to which the basic block belongs, and then from there binary search
    /// over all basic blocks allocated in that slab.
    extern "C" void **granary_find_fragment_slab(uintptr_t fragment_addr) {
        const unsigned index = (fragment_addr - GRANARY_EXEC_START) / FRAGMENT_SLAB_SIZE;
        return &(FRAGMENT_SLABS[index]);
    }


    void init_code_cache(void) {
#if CONFIG_ENV_KERNEL
        kernel_make_pages_executable(
            &(EXECUTABLE_AREA[0]),
            &(EXECUTABLE_AREA[NUM_EXEC_PAGES])
        );
#else
        mprotect(
            &(EXECUTABLE_AREA[0]), CODE_CACHE_SIZE,
            PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

        GRANARY_EXEC_START = reinterpret_cast<uintptr_t>(&(EXECUTABLE_AREA[0]));
        GRANARY_EXEC_END = GRANARY_EXEC_START + CODE_CACHE_SIZE;

        CODE_CACHE_END = GRANARY_EXEC_START;
        WRAPPER_START = GRANARY_EXEC_END - _1_MB;
        WRAPPER_END = WRAPPER_START;
        GEN_CODE_START = WRAPPER_START;
    }


    void *global_allocate_executable(uintptr_t size, int where) {

        uintptr_t mem = 0;
        switch(where) {

        // Code cache pages are allocated from the beginning
        case EXEC_CODE_CACHE:
            mem = __sync_fetch_and_add(&CODE_CACHE_END, size);
            if((mem + size) > GEN_CODE_START) {
                granary_fault();
            }
            break;

        // Gencode pages are allocated from near the end
        case EXEC_GEN_CODE:
            mem = __sync_sub_and_fetch(&GEN_CODE_START, size);
            if(mem < CODE_CACHE_END) {
                granary_fault();
            }
            break;

        // Wrapper entry points are allocated from the end in a
        // fixed-size buffer.
        case EXEC_WRAPPER:
            mem = __sync_fetch_and_add(&WRAPPER_END, size);
            if((mem + size) > GRANARY_EXEC_END) {
                granary_fault();
            }
            break;

        default:
            //printk("[granary] Unknown executable allocation type!\n\n");
            granary_fault();
            break;
        }

        return memset((void *) mem, 0xCC, size);
    }


    void global_free_executable(void *, uintptr_t) {
        // NO-OP.
    }
}}

namespace granary {
    bool is_code_cache_address(const const_app_pc addr_) {
        const uintptr_t addr(reinterpret_cast<uintptr_t>(addr_));
        return GRANARY_EXEC_START <= addr && addr < detail::CODE_CACHE_END;
    }


    bool is_wrapper_address(const const_app_pc addr_) {
        const uintptr_t addr(reinterpret_cast<uintptr_t>(addr_));
        return detail::WRAPPER_START <= addr && addr < detail::WRAPPER_END;
    }


    bool is_gencode_address(const const_app_pc addr_) {
        const uintptr_t addr(reinterpret_cast<uintptr_t>(addr_));
        return detail::GEN_CODE_START <= addr && addr < detail::WRAPPER_START;
    }
}


/// Add some illegal detach points.
GRANARY_DETACH_POINT_ERROR(granary::detail::global_allocate_executable)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_free_executable)


#define ENABLE_SPLITTING 0


namespace granary { namespace detail {

    enum {
        HEAP_SIZE = _1_MB * 256,
        MIN_SCALE = 3,
        UNSIGNED_LONG_NUM_BITS = sizeof(uintptr_t) * 8,
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
    static uint8_t HEAP[HEAP_SIZE] = {0};


    /// Bump pointer.
    static std::atomic<unsigned> HEAP_INDEX(ATOMIC_VAR_INIT(0));


    /// Free lists, based on size.
    static free_list FREE_LISTS[NUM_FREE_LISTS];


    /// Returns the log base 2 of a number.
    static inline uint32_t log_base_2(uintptr_t x) {
        return ((UNSIGNED_LONG_NUM_BITS - 1) - __builtin_clzl(x));
    }


    /// Return the smallest power of two value greater than x.
    static inline uintptr_t next_power_of_2(uintptr_t x) {
        return 1UL << (UNSIGNED_LONG_NUM_BITS - __builtin_clzl(x - 1));
    }


    /// Round the size of some data.
    __attribute__((hot))
    static uintptr_t allocation_size(uintptr_t size) {
        if(MIN_OBJECT_SIZE > size) {
            return MIN_OBJECT_SIZE;
        }

        const uintptr_t high_power(next_power_of_2(size));
        const uintptr_t low_power(high_power / 2);

        return (low_power >= size) ? low_power : high_power;
    }


    /// Returns true of an address is a head address.
    __attribute__((hot))
    inline static bool is_heap_address(void *ptr_) {
        const uint8_t *ptr(reinterpret_cast<uint8_t *>(ptr_));
        return ptr >= &(HEAP[0]) && ptr < &(HEAP[HEAP_SIZE]);
    }


#if ENABLE_SPLITTING
    /// Try to split a large heap object into a smaller heap object.
    __attribute__((hot))
    static free_object *split_large_object(unsigned scale) {
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
    void *global_allocate(uintptr_t size_) {

        ASSERT(0 < size_);

        const uintptr_t size(allocation_size(size_));
        const unsigned scale(log_base_2(size) - MIN_SCALE);

        ASSERT(scale <= NUM_FREE_LISTS);
        ASSERT(size >= size_);

        for(;;) {
            if(!FREE_LISTS[scale].head.load()) {

                const unsigned curr_heap_index(HEAP_INDEX.fetch_add(size));
                const uintptr_t next_heap_index(curr_heap_index + size);

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

        return nullptr;
    }


    /// Free some memory back to the heap.
    __attribute__((hot))
    void global_free(void *addr, uintptr_t size_) {

        if(!is_heap_address(addr)) {
            return;
        }

        const uintptr_t size(allocation_size(size_));
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


/// Add some illegal detach points.
GRANARY_DETACH_POINT_ERROR(granary::detail::global_allocate)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_free)


extern "C" {

    /// DynamoRIO-compatible heap allocation

    void *granary_heap_alloc(void *, unsigned long long size) {
        granary::cpu_state_handle cpu;
        return cpu->transient_allocator.allocate_untyped(16, size);
    }

    void granary_heap_free(void *, void *, unsigned long) {
    }

    /// Return temporarily allocated space for an instruction.
    void *granary_heap_alloc_temp_instr(void) {
        granary::cpu_state_handle cpu;
        return cpu->instruction_allocator.allocate_array<uint8_t>(32);
    }
}


/// Add some illegal detach points.
GRANARY_DETACH_POINT_ERROR(granary_heap_alloc)
GRANARY_DETACH_POINT_ERROR(granary_heap_free)
GRANARY_DETACH_POINT_ERROR(granary_heap_alloc_temp_instr)

