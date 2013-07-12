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


    thread_state_handle::thread_state_handle(cpu_state_handle cpu_) throw()
        : state(&(cpu_->thread_data))
    { }


    cpu_state_handle::cpu_state_handle(void) throw()
        : state(CPU_STATE)
    {
        if(!state) {
            state = CPU_STATE = allocate_memory<cpu_state>();
        }
    }

    /// This pre-processor check exists only to "hint" to Eclipse not to resolve
    /// names to the following functions when instrumenting kernel code. This
    /// does not affect the runtime.
#if !GRANARY_IN_KERNEL

    extern "C" bool is_code_cache_address(app_pc) throw() {
        return true; // TODO
    }

    extern "C" bool is_wrapper_address(app_pc) throw() {
        return true; // TODO
    }

    extern "C" bool is_gencode_address(app_pc) throw() {
        return true; // TODO
    }

#endif /* GRANARY_IN_KERNEL */
}



