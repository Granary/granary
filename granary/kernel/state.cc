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
    extern granary::cpu_state **kernel_get_cpu_state(granary::cpu_state *[]);
    extern void kernel_run_on_each_cpu(void (*func)(void *), void *thunk);
}


namespace granary {


    /// Manually manage our own per-cpu state.
    static cpu_state *CPU_STATES[256] = {nullptr};


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(*kernel_get_cpu_state(CPU_STATES))
    { }


    /// Initialise the per-cpu state, including a new interrupt descriptor
    /// table.
    void init_cpu_state(void *) throw() {
        cpu_state **state_ptr(kernel_get_cpu_state(CPU_STATES));
        *state_ptr = new cpu_state;

        cpu_state *state(*state_ptr);
        state->interrupt_delay_handler = reinterpret_cast<app_pc>(
            global_state::FRAGMENT_ALLOCATOR-> \
                allocate_untyped(16, INTERRUPT_DELAY_CODE_SIZE));
    }


    /// Initialise the state.
    void cpu_state_handle::init(void) throw() {
        kernel_run_on_each_cpu(init_cpu_state, nullptr);
    }


    /// Gets a handle to the current thread state.
    thread_state_handle::thread_state_handle(void) throw()
        : state(nullptr)
    { }
}
