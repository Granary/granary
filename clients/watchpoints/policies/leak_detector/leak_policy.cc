/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.cc
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/policies/leak_detector/leak_policy.h"

using namespace granary;


namespace client { namespace wp {

    /// Initialise the leak policy enter.
    STATIC_INITIALISE({
        app_leak_policy_enter::init();
    })

    void app_leak_policy_enter::init() throw() {
        granary::printf("leak policy init !!!!!!!!!!!!!!!!\n");
    }



#if CONFIG_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state app_leak_policy_enter::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }

    interrupt_handled_state app_leak_policy_exit::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }

    interrupt_handled_state app_leak_policy_continue::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

}

} /* client namespace */
