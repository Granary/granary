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

        (void) thread;
    }


    /// static initialization of global fragment allocator
    bump_pointer_allocator<detail::global_fragment_allocator_config> \
        global_state::fragment_allocator;
}
