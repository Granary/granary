/*
 * state.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/state.h"

namespace granary {


    /// Notify that we're entering granary.
    void enter(cpu_state_handle &cpu, thread_state_handle &thread) throw() {
        cpu->transient_allocator.free_all();
        cpu->instruction_allocator.free_all();

        (void) thread;
    }

    /// Static initialisation of global fragment allocator
    static_data<
        bump_pointer_allocator<detail::global_fragment_allocator_config>
    > global_state::FRAGMENT_ALLOCATOR;


    STATIC_INITIALISE({
        global_state::FRAGMENT_ALLOCATOR.construct();
    })
}
