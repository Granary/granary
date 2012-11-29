/*
 * state.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/state.h"

namespace granary {

    /// Hack to ensure static initializers are compiled.
    static_init_list STATIC_LIST_HEAD;


    /// Notify that we're entering granary.
    void enter(cpu_state_handle &cpu, thread_state_handle &thread) throw() {
        cpu->transient_allocator.free_all();

        (void) thread;
    }
}
