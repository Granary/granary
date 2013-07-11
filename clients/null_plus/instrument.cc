/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-07-10
 *      Author: Peter Goodman
 */


#include "clients/null_plus/instrument.h"

namespace client {

    /// Instruction a basic block.
    granary::instrumentation_policy null_plus_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &ls
    ) throw() {
        granary::instrumentation_policy policy(granary::START_POLICY);
        policy.force_attach(true);

        for(granary::instruction in(ls.first()); in.is_valid(); in = in.next()) {
            if(in.is_cti()) {
                in.set_policy(policy);
            }
        }

        return granary::policy_for<null_plus_policy>();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy null_plus_policy::visit_host_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        return visit_app_instructions(cpu, bb, ls);
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state null_plus_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }

#endif
}
