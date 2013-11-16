/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-10-22
 *      Author: Peter Goodman
 */


#include "clients/instr_dist/instrument.h"

using namespace granary;

namespace client {


    extern instruction_log INSTRUCTION_DIST[];


    /// Instruction a basic block.
    granary::instrumentation_policy instr_dist_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &ls
    ) throw() {
        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
            instruction_log &log(INSTRUCTION_DIST[in.op_code()]);

            const unsigned next_offset(log.num_instances.fetch_add(1));
            const app_pc pc(in.pc_or_raw_bytes());
            if(!pc) {
                continue;
            }

            log.instances[
                next_offset % instruction_log::NUM_RECORDED_INSTANCES] = pc;
        }
        return *this;
    }


    /// Instruction a basic block.
    granary::instrumentation_policy instr_dist_policy::visit_host_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        return visit_app_instructions(cpu, bb, ls);
    }


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state instr_dist_policy::handle_interrupt(
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


