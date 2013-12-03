/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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
    __thread cpu_state *CPU_STATE(nullptr);

    extern "C" uint64_t *granary_get_private_stack_top(void)
    {
        return &(cpu_state_handle()->stack.top[0]);
    }

    thread_state_handle::thread_state_handle(safe_cpu_access_zone) throw()
        : state(&(cpu_state_handle()->thread_data))
    { }


    cpu_state_handle::cpu_state_handle(void) throw()
        : state(CPU_STATE)
    {
        if(!state) {
            state = CPU_STATE = allocate_memory<cpu_state>();
        }
    }
}



