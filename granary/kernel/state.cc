/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include <atomic>

#include "granary/state.h"
#include "granary/types.h"


extern "C" {
    extern granary::cpu_state *get_percpu_state(void *);
}


namespace granary {


    static void *CPU_STATES;


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(get_percpu_state(CPU_STATES))
    { }


    void init_cpu_state(void *) throw() {
        new (get_percpu_state(CPU_STATES)) cpu_state;
    }


    /// Initialise the state.
    void cpu_state_handle::init(void) throw() {

        CPU_STATES = types::__alloc_percpu(
            sizeof(cpu_state), alignof(cpu_state));

        types::on_each_cpu(init_cpu_state, nullptr, 1);
    }


    /// Gets a handle to the current thread state.
    thread_state_handle::thread_state_handle(void) throw()
        : state(nullptr)
    { }
}
