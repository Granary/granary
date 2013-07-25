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


    /// Represents a pseudo-CPU state handle, meant to document that it's safe
    /// to access CPU state.
    struct safe_cpu_access_zone { };


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


        /// Implicit conversion operator for marking a location as safe to
        /// access CPU state by virtue of us having already accessed CPU-private
        /// state.
        inline operator safe_cpu_access_zone(void) const throw() {
            return safe_cpu_access_zone();
        }


        /// Free up transient CPU state.
        void free_transient_allocators(void) throw();
    };


    /// A handle on thread state. Implemented differently in the kernel and in
    /// user space.
    struct thread_state_handle {
    private:

        IF_KERNEL( thread_state **state_ptr; )
        IF_KERNEL( void init(void) throw(); )

        thread_state *state;

        /// Not allowed to get thread state without CPU state.
        thread_state_handle(void) throw() = delete;

    public:


        /// Initialise some thread-private state.
        __attribute__((hot))
        thread_state_handle(safe_cpu_access_zone) throw();


        FORCE_INLINE thread_state *operator->(void) throw() {

#if GRANARY_IN_KERNEL
            // Used to lazily initialise thread state on access.
            if(unlikely(nullptr != state_ptr)) {
                init();
            }
#endif
            return state;
        }
    };
}

#endif /* STATE_HANDLE_H_ */
