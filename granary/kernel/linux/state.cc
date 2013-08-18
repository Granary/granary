/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.cc
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/types.h"

extern "C" {
    /// Used to access thread-private data.
    extern void *kernel_get_thread_state(void);
}

namespace granary {

    /// Used to access thread-local data. The dependency on a valid CPU state
    /// handle implies that thread state should only be accessed when interrupts
    /// are disabled (where it's safe to access CPU-private data).
    thread_state_handle::thread_state_handle(safe_cpu_access_zone) throw() {

        enum {
            STATE_SIZE = sizeof((new types::task_struct)->granary)
        };

        static_assert(sizeof(uintptr_t) <= STATE_SIZE,
            "The size of the `granary` field in the Linux kernel's `struct "
            "task_struct` must be at least `sizeof(uintptr_t)`.");

        // Optimisation if the size of the thread state can fit entirely within
        // the thread state pointer.
        if(sizeof(thread_state) <= STATE_SIZE) {
            state = unsafe_cast<thread_state *>(kernel_get_thread_state());
            state_ptr = nullptr;

        // Thread state is too big to fit into 8 bytes.
        } else {
            state_ptr = unsafe_cast<thread_state **>(kernel_get_thread_state());
            state = *state_ptr;
            if(state) {
                state_ptr = nullptr;
            }
        }
    }
}

