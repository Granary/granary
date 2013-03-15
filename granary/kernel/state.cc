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

namespace granary {

    namespace {
        cpu_state *CPU_STATE __attribute__((section (".data..percpu")));
    }


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(CPU_STATE)
    { }


    void init_cpu_state(void *) throw() {
        CPU_STATE = new cpu_state;
    }


    /// Initialise the state.
    void cpu_state_handle::init(void) throw() {
        types::on_each_cpu(init_cpu_state, nullptr, 1);
    }


    /// Gets a handle to the current thread state.
    thread_state_handle::thread_state_handle(void) throw()
        : state(nullptr)
    { }
}
