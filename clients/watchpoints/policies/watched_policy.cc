/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * watched_policy.cc
 *
 *  Created on: 2013-05-12
 *      Author: Peter Goodman
 */


#include "clients/watchpoints/policies/watched_policy.h"

using namespace granary;

namespace client { namespace wp {


    void watched_policy::visit_read(
        granary::basic_block_state &,
        instruction_list &,
        watchpoint_tracker &,
        unsigned
    ) throw() { }


    void watched_policy::visit_write(
        granary::basic_block_state &,
        instruction_list &,
        watchpoint_tracker &,
        unsigned
    ) throw() { }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state watched_policy::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector
    ) throw() {
        return INTERRUPT_DEFER;
    }
} /* wp namespace */


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    interrupt_handled_state handle_kernel_interrupt(
        cpu_state_handle &cpu,
        thread_state_handle &thread,
        interrupt_stack_frame &isf,
        interrupt_vector vector
    ) throw() {
        if(VECTOR_GENERAL_PROTECTION == vector && !isf.error_code) {

            instrumentation_policy policy(START_POLICY);
            policy.in_host_context();

            mangled_address target(isf.instruction_pointer, policy);

            isf.instruction_pointer = code_cache::find(cpu, thread, target);
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }
#else
} /* wp namespace */
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

} /* client namespace */


