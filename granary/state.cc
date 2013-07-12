/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/state.h"

namespace granary {


    /// Notify that we're entering granary.
    void enter(cpu_state_handle cpu) throw() {
        if (1 == *granary_get_private_stack_top()) {
            cpu.free_transient_allocators();
        }
    }


    /// Free up transient CPU state.
    void cpu_state_handle::free_transient_allocators(void) throw() {
        state->transient_allocator.free_all();
        state->instruction_allocator.free_all();
    }


    /// Static initialisation of global fragment allocator
    static_data<
        bump_pointer_allocator<detail::global_fragment_allocator_config>
    > global_state::FRAGMENT_ALLOCATOR;


    static_data<
        bump_pointer_allocator<detail::wrapper_allocator_config>
    > global_state::WRAPPER_ALLOCATOR;


    STATIC_INITIALISE_ID(global_state, {
        global_state::FRAGMENT_ALLOCATOR.construct();
        global_state::WRAPPER_ALLOCATOR.construct();
    })
}
