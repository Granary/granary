/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: Oct 25, 2013
 *      Author: pag
 */


#include "clients/even_odd/instrument.h"

namespace client {

    /// Instruction a basic block.
    granary::instrumentation_policy even_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<odd_policy>();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy even_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<odd_policy>();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy odd_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<even_policy>();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy odd_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<even_policy>();
    }


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code.
    granary::interrupt_handled_state even_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }


    /// Handle an interrupt in module code.
	granary::interrupt_handled_state odd_policy::handle_interrupt(
		granary::cpu_state_handle,
		granary::thread_state_handle,
		granary::basic_block_state &,
		granary::interrupt_stack_frame &,
		granary::interrupt_vector
	) throw() {
		return granary::INTERRUPT_DEFER;
	}


    /// Handle an interrupt in kernel code.
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


