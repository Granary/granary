/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/globals.h"
#include "granary/state.h"

extern "C" {


    /// Get access to the CPU-private state.
    extern granary::cpu_state **kernel_get_cpu_state(granary::cpu_state *[]);


    /// Used to run a function on each CPU.
    extern void kernel_run_on_each_cpu(void (*func)(void *), void *thunk);
}


namespace granary {


    /// Manually manage our own per-cpu state.
    cpu_state *CPU_STATES[256] = {nullptr};

    extern "C" uint64_t *granary_get_private_stack_top(void)
    {
        return &((*kernel_get_cpu_state(CPU_STATES))->percpu_stack).depth;
    }

    namespace detail {
        struct dummy_thread_state : public client::thread_state {
            uint64_t dummy_val;
        };
    }


    /// Figure out the size of the CPU state.
    enum {
        DUMMY_THREAD_STATE_SIZE = sizeof(detail::dummy_thread_state),
        ALIGNED_THREAD_STATE_SIZE = sizeof(granary::thread_state)
                                  + ALIGN_TO(sizeof(granary::thread_state), 8),
        THREAD_STATE_SIZE = (sizeof(uint64_t) == DUMMY_THREAD_STATE_SIZE)
                          ? 0
                          : ALIGNED_THREAD_STATE_SIZE
    };


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(*kernel_get_cpu_state(CPU_STATES))
    { }


    /// Initialise the CPU state.
    void cpu_state_handle::init(void) throw() {
        cpu_state **state_ptr(kernel_get_cpu_state(CPU_STATES));
        *state_ptr = allocate_memory<cpu_state>();

        cpu_state *state(*state_ptr);
        state->interrupt_delay_handler = reinterpret_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR-> \
                allocate_untyped(16, INTERRUPT_DELAY_CODE_SIZE));
    }


    /// Initialise the thread state.
    void thread_state_handle::init(void) throw() {
        if(sizeof(thread_state) > sizeof(uintptr_t)) {
            state = allocate_memory<thread_state>();
            *state_ptr = state;
            state_ptr = nullptr;
        }
    }


    /// Note: thread_state_handle::thread_state_handle is in the OS-specific
    ///       state.cc file.
}
