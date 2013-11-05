/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * allocator.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/detach.h"
#include "granary/spin_lock.h"

#include <cstdlib>
#include <stdint.h>

#define PROT_ALL (~0)
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

extern "C" {

#   include <sys/mman.h>
#   include <unistd.h>

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

namespace granary { namespace detail {

    enum executable_memory_kind {
        EXEC_CODE_CACHE = 0,
        EXEC_GEN_CODE = 1,
        EXEC_WRAPPER = 2
    };


    enum {
        _1_MB = 1048576,
        CODE_CACHE_SIZE = 40 * _1_MB,
        _1_P = 4096,

        // Maximum size of the part of the code cache containing basic blocks /
        // fragments.
        FRAGMENT_CACHE_MAX_SIZE = CODE_CACHE_SIZE - _1_MB,

        // Keep this consistent with `granary/state.h`,
        // fragment_allocator_config::SLAB_SIZE
        FRAGMENT_SLAB_SIZE = _1_P * 8,

        // Maximum number of fragment slabs.
        MAX_NUM_FRAGMENT_SLABS = FRAGMENT_CACHE_MAX_SIZE / FRAGMENT_SLAB_SIZE
    };


    struct page {
        uint8_t data[4096];
    } __attribute__((aligned (4096)));


    static page EXECUTABLE_AREA[CODE_CACHE_SIZE / sizeof(page)];


    /// Exposed for the C++ side of things to see for the debug memset hot-patcher.
    unsigned long EXEC_START = 0;
    static unsigned long EXEC_END = 0;


    /// Layout of the executable region:
    ///
    ///  EXEC_START                GEN_CODE_START   WRAPPER_START       EXEC_END
    ///      |--------------->              <--------------|-------->      |
    ///                CODE_CACHE_END                          WRAPPER_END
    ///
    unsigned long CODE_CACHE_END = 0;
    static unsigned long GEN_CODE_START = 0;
    static unsigned long WRAPPER_START = 0;
    static unsigned long WRAPPER_END = 0;


    static void *FRAGMENT_SLABS[MAX_NUM_FRAGMENT_SLABS] = {NULL};


    /// Given an arbitrary address into a basic block, we want to be able to find
    /// the associated basic block info / meta-data. The approach is to first find
    /// the slab to which the basic block belongs, and then from there binary search
    /// over all basic blocks allocated in that slab.
    extern "C" void **granary_find_fragment_slab(unsigned long fragment_addr) {
        const unsigned index = (fragment_addr - EXEC_START) / FRAGMENT_SLAB_SIZE;
        return &(FRAGMENT_SLABS[index]);
    }


    void init_code_cache(void) throw() {
        mprotect(
            &(EXECUTABLE_AREA[0]), CODE_CACHE_SIZE,
            PROT_READ | PROT_WRITE | PROT_EXEC);

        EXEC_START = reinterpret_cast<unsigned long>(&(EXECUTABLE_AREA[0]));
        EXEC_END = EXEC_START + CODE_CACHE_SIZE;

        CODE_CACHE_END = EXEC_START;
        WRAPPER_START = EXEC_END - _1_MB;
        WRAPPER_END = WRAPPER_START;
        GEN_CODE_START = WRAPPER_START;
    }


    void *global_allocate_executable(unsigned long size, int where) throw() {

        unsigned long mem = 0;
        switch(where) {

        // code cache pages are allocated from the beginning
        case EXEC_CODE_CACHE:
            mem = __sync_fetch_and_add(&CODE_CACHE_END, size);
            if((mem + size) > GEN_CODE_START) {
                granary_fault();
            }
            break;

        // gencode pages are allocated from near the end
        case EXEC_GEN_CODE:
            mem = __sync_sub_and_fetch(&GEN_CODE_START, size);
            if(mem < CODE_CACHE_END) {
                granary_fault();
            }
            break;

        // wrapper entry points are allocated from the end in a fixed-size buffer.
        case EXEC_WRAPPER:
            mem = __sync_fetch_and_add(&WRAPPER_END, size);
            if((mem + size) > EXEC_END) {
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


    extern "C" {

        /// granary::is_code_cache_address
        bool is_code_cache_address(app_pc addr_) throw() {
            const unsigned long addr(reinterpret_cast<unsigned long>(addr_));
            return EXEC_START <= addr && addr < CODE_CACHE_END;
        }


        /// granary::is_wrapper_address
        bool is_wrapper_address(app_pc addr_) throw() {
            const unsigned long addr(reinterpret_cast<unsigned long>(addr_));
            return WRAPPER_START <= addr && addr < WRAPPER_END;
        }


        /// granary::is_gencode_address
        bool is_gencode_address(app_pc addr_) throw() {
            const unsigned long addr(reinterpret_cast<unsigned long>(addr_));
            return GEN_CODE_START <= addr && addr < WRAPPER_START;
        }
    }


    void global_free_executable(void *, unsigned long) throw() {
        // NO-OP.
    }


    void *global_allocate(unsigned long size) throw() {
        return malloc(size);
    }


    void global_free(void *addr, unsigned long) throw() {
        free(addr);
    }
}}


/// Add some illegal detach points.
GRANARY_DETACH_POINT_ERROR(granary::detail::global_allocate)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_allocate_executable)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_free)
GRANARY_DETACH_POINT_ERROR(granary::detail::global_free_executable)
