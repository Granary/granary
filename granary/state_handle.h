/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state_handle.h
 *
 *  Created on: 2013-07-01
 *      Author: Peter Goodman
 */

#ifndef STATE_HANDLE_H_
#define STATE_HANDLE_H_

#include "granary/globals.h"

namespace granary {

    struct cpu_state;
    struct thread_state;
    struct thread_state_handle;


    /// Define a CPU state handle; in user space, the cpu state is also thread
    /// private.
    struct cpu_state_handle {
    private:

        IF_KERNEL( friend struct thread_state_handle; )

        cpu_state *state;

    public:

        __attribute__((hot))
        cpu_state_handle(void) throw();


        FORCE_INLINE
        cpu_state *operator->(void) throw() {
            return state;
        }

        IF_KERNEL( static void init(void) throw(); )
    };


    /// A handle on thread state. Implemented differently in the kernel and in
    /// user space.
    struct thread_state_handle {
    private:

        thread_state *state;

        /// Not allowed to get thread state without CPU state.
        thread_state_handle(void) throw() = delete;

        /// Used to track if this CPU has a thread state associated with it, and
        /// if not, then we can correctly initialise the thread state on the
        /// first request (operator->), and update the CPU state accordingly.
        IF_KERNEL( cpu_state *cpu; )
        IF_KERNEL( void init(void); )

    public:

        /// This constructor must be used when accessing Granary from within
        /// Granary's stack.
        __attribute__((hot))
        thread_state_handle(cpu_state_handle) throw();


        FORCE_INLINE
        thread_state *operator->(void) throw() {
#if GRANARY_IN_KERNEL
            if(unlikely(nullptr != cpu)) {
                init();
            }
#endif
            return state;
        }
    };
}

#endif /* STATE_HANDLE_H_ */
