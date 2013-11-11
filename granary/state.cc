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
        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();
        cpu->current_fragment_allocator = &(cpu->fragment_allocator);
        IF_TEST( cpu->in_granary = true; )
    }


    /// Free up transient CPU state.
    void cpu_state_handle::free_transient_allocators(void) throw() {

        // The typical model for Granary is to free all transiently allocated
        // state on "entry" to Granary. We consider Granary entrypoints to only
        // happen after Granary has been initialised. This function is often
        // useful during Granary's initialisation as it allows us to deallocate
        // in bulk. After initialisation, we only want one such deallocation
        // per entry into Granary, because more indicates a misuse.
        ASSERT(!state->in_granary);

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
