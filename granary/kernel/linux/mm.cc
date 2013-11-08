/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * mm.cc
 *
 *  Created on: 2013-11-06
 *      Author: Peter Goodman
 */

/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * syscall.cc
 *
 *  Created on: 2013-11-06
 *          Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/detach.h"
#include "granary/state.h"
#include "granary/policy.h"
#include "granary/code_cache.h"

#if CONFIG_ENABLE_TRACE_ALLOCATOR \
&& !CONFIG_TRACE_FUNCTIONAL_UNITS \
&& !CONFIG_TRACE_CPUS

namespace granary {


    /// Initialise an individual system call. This will create a new allocator
    /// for basic blocks executed by that system call, thus contributing to
    /// "tracing" that basic block.
    static void init_mm(uintptr_t addr_) throw() {

        app_pc addr(reinterpret_cast<app_pc>(addr_));
        cpu_state_handle cpu;

        // Create a syscall-specific memory allocator.
        generic_fragment_allocator *allocator(
            allocate_memory<generic_fragment_allocator>());

        // Free up some memory.
        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();
        cpu->current_fragment_allocator = allocator;

        code_cache::find(cpu, mangled_address(addr, START_POLICY));
    }


    STATIC_INITIALISE_ID(mm_entrypoints, {
#ifdef DETACH_ADDR___kmalloc
        init_mm(DETACH_ADDR___kmalloc);
#endif
#ifdef DETACH_ADDR_kmem_cache_alloc
        init_mm(DETACH_ADDR_kmem_cache_alloc);
#endif
    })
}

#endif /* CONFIG_INSTRUMENT_HOST && CONFIG_ENABLE_TRACE_ALLOCATOR */

