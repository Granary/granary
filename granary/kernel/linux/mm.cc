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

/// We don't combine MM tracing with functional unit tracing because then we'll
/// leak an allocator, and we won't get the desired result anyway.

#if CONFIG_ENABLE_TRACE_ALLOCATOR \
&& CONFIG_TRACE_ALLOCATE_MM \
&& !CONFIG_TRACE_ALLOCATE_FUNCTIONAL_UNITS

namespace granary {


    /// Initialise an individual system call. This will create a new allocator
    /// for basic blocks executed by that system call, thus contributing to
    /// "tracing" that basic block.
    static void init_mm(uintptr_t addr_) throw() {

        app_pc addr(reinterpret_cast<app_pc>(addr_));
        cpu_state_handle cpu;

        // Free up some memory.
        IF_TEST( cpu->in_granary = false; )
        enter(cpu);

        // Create an allocator-specific memory allocator.
        generic_fragment_allocator *allocator(
            allocate_memory<generic_fragment_allocator>());
        cpu->current_fragment_allocator = allocator;

        instrumentation_policy policy(START_POLICY);
        policy.begins_functional_unit(true);
        policy.in_host_context(true);
        policy.return_address_in_code_cache(true);

        code_cache::find(cpu, mangled_address(addr, policy));
    }


    STATIC_INITIALISE_ID(trace_alloc_mm_entrypoints, {
#ifdef DETACH_ADDR___kmalloc
        init_mm(DETACH_ADDR___kmalloc);
#endif
#ifdef DETACH_ADDR_kmem_cache_alloc
        init_mm(DETACH_ADDR_kmem_cache_alloc);
#endif
    })
}

#endif
