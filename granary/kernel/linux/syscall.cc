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

#if CONFIG_ENABLE_TRACE_ALLOCATOR && !CONFIG_TRACE_CPUS
#   define IF_CUSTOM_ALLOCATOR(...) __VA_ARGS__
#else
#   define IF_CUSTOM_ALLOCATOR(...)
#endif

namespace granary {

#if defined(DETACH_ADDR_sys_call_table) \
&& DETACH_LENGTH_sys_call_table \
&& CONFIG_INSTRUMENT_HOST

    enum {
        NUM_ENTRIES = DETACH_LENGTH_sys_call_table / sizeof(void *)
    };


    /// Actual shadow syscall table.
    static app_pc SYSCALL_TABLE[NUM_ENTRIES] = {nullptr};


    /// Creates a shadow system call table.
    STATIC_INITIALISE_ID(duplicate_syscall_table, {

        const app_pc *native_table(unsafe_cast<app_pc *>(
            DETACH_ADDR_sys_call_table));

        // Starting policy.
        instrumentation_policy policy(START_POLICY);
        policy.begins_functional_unit(true);
        policy.in_host_context(true);

        cpu_state_handle cpu;

        for(unsigned i(0); i < NUM_ENTRIES; ++i) {

            // Free up some memory.
            IF_TEST( cpu->in_granary = false; )
            enter(cpu);

            // If we're using the trace allocator then add an allocator in one
            // of two cases:
            //      1) We're tracing system call entrypoints (default trace
            //         allocator strategy).
            //      2) We're tracing functional units.
            IF_CUSTOM_ALLOCATOR( cpu->current_fragment_allocator =
                allocate_memory<generic_fragment_allocator>(); )

            mangled_address am(native_table[i], policy);
            SYSCALL_TABLE[i] = code_cache::find(cpu, am);
        }

    })

    extern "C" {
        intptr_t NATIVE_SYSCALL_TABLE = DETACH_ADDR_sys_call_table;
        intptr_t SHADOW_SYSCALL_TABLE = reinterpret_cast<uintptr_t>(
            &(SYSCALL_TABLE[0]));
    }

#else
    extern "C" {
        intptr_t NATIVE_SYSCALL_TABLE = 0;
        intptr_t SHADOW_SYSCALL_TABLE = 0;
    }
#endif
}


