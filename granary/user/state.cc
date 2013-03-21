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
    __thread thread_state *THREAD_STATE(nullptr);
    __thread cpu_state *CPU_STATE(nullptr);


    thread_state_handle::thread_state_handle(void) throw()
        : state(THREAD_STATE)
    {
        if(!state) {
            state = THREAD_STATE = new thread_state;
        }
    }


    cpu_state_handle::cpu_state_handle(void) throw()
        : state(CPU_STATE)
    {
        if(!state) {
            state = CPU_STATE = new cpu_state;
        }
    }

    bool is_code_cache_address(app_pc) throw() {
        return true; // TODO
    }

    bool is_wrapper_address(app_pc) throw() {
        return true; // TODO
    }
}



