/*
 * instrument.cc
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#include "clients/instrument.h"

namespace client {

    /// Instruction a basic block.
    granary::instrumentation_policy null_policy::visit_basic_block(
        granary::cpu_state_handle &,
        granary::thread_state_handle &,
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
