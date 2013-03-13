/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include <atomic>

#include "granary/state.h"

namespace granary {

    namespace {
        cpu_state *CPU_STATE __attribute__((section (".data..percpu")));
    }


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(CPU_STATE)
    { }


    /// Gets a handle to the current thread state.
    thread_state_handle::thread_state_handle(void) throw()
        : state(nullptr)
    { }
}
