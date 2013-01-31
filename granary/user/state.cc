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

    /// User space thread-local storage.
    __thread thread_state *THREAD_STATE = new thread_state;
    __thread cpu_state *CPU_STATE = new cpu_state;


    thread_state_handle::thread_state_handle(void) throw()
        : state(THREAD_STATE)
    { }


    cpu_state_handle::cpu_state_handle(void) throw()
        : state(CPU_STATE)
    { }
}



