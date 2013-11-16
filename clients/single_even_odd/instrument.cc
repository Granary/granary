/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-10-26
 *      Author: Peter Goodman
 */


#include "clients/single_even_odd/instrument.h"

using namespace granary;

namespace client {

    /// Instruction a basic block.
    granary::instrumentation_policy even_odd_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &ls
    ) throw() {

        // Don't recursively instrument indirect CTIs as basic blocks.
        if(ls.is_stub()) {
            return granary::policy_for<even_odd_policy>();
        }

        instruction even(ls.prepend(label_()));
        instruction go_to_even(ls.prepend(label_()));
        instruction odd(ls.append(label_()));
        instruction test(ls.prepend(label_()));

        // Create two versions of the instruction list.
        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
            if(odd == in) {
                break;
            }
            ls.append(in.clone());
        }

        // Switch between even or odd at the beginning of the basic block.
        ls.insert_before(test, push_(reg::rcx));

        // Read an uninitialised value from the stack and branch based on that.
        ls.insert_before(test, mov_ld_(reg::rcx, reg::rsp[-8]));
        ls.insert_before(test, jecxz_(instr_(go_to_even)));
        ls.insert_before(test, jmp_(instr_(odd)));

        // go_to_even is here and falls-through to even.

        // Write 1 onto the stack, then pop RCX and fall through into the
        // "even" side of the instrumentation.
        ls.insert_before(even, mov_imm_(reg::ecx, int32_(1)));
        ls.insert_before(even, mov_st_(reg::rsp[-8], reg::rcx));
        ls.insert_before(even, pop_(reg::rcx));

        // Write 0 onto the stack, then pop RCX and fall through into the
        // "odd" size of the instrumentation.
        odd = ls.insert_after(odd, mov_imm_(reg::ecx, int32_(0)));
        odd = ls.insert_after(odd, mov_st_(reg::rsp[-8], reg::rcx));
        ls.insert_after(odd, pop_(reg::rcx));

        return granary::policy_for<even_odd_policy>();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy even_odd_policy::visit_host_instructions(
        granary::cpu_state_handle cpu,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        return visit_app_instructions(cpu, bb, ls);
    }


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code.
    granary::interrupt_handled_state even_odd_policy::handle_interrupt(
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


