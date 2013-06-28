/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#include "clients/null_policy.h"

namespace client {

    /// Instruction a basic block.
    granary::instrumentation_policy null_policy::visit_app_instructions(
        granary::cpu_state_handle &,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<null_policy>();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy null_policy::visit_host_instructions(
        granary::cpu_state_handle &,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<null_policy>();
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state null_policy::handle_interrupt(
        granary::cpu_state_handle &,
        granary::thread_state_handle &,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle &,
        granary::thread_state_handle &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }

#endif
}
