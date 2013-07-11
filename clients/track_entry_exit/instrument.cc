/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#include "granary/test.h"

#include "clients/track_entry_exit/instrument.h"

namespace client {


    /// This function is called on entry to instrumented code.
    static void on_enter_code_cache(void) throw() {
        granary::printf("Entering the code cache.\n");
    }


    /// This function is called when we exit from the code cache.
    static void on_exit_code_cache(void) throw() {
        granary::printf("Exiting the code cache.\n");
    }


    static granary::app_pc EVENT_ENTER_MODULE = nullptr;
    static granary::app_pc EVENT_EXIT_MODULE = nullptr;


    void init(void) throw() {
        using namespace granary;
        EVENT_ENTER_MODULE = generate_clean_callable_address(
            &on_enter_code_cache);

        EVENT_EXIT_MODULE = generate_clean_callable_address(
            &on_exit_code_cache);
    }


    /// Instrument the first entrypoint into instrumented code.
    static void instrument_entry_to_code_cache(
        granary::instruction_list &ls
    ) throw() {
        using namespace granary;
        instruction in(ls.first());
        in = ls.insert_before(in, label_());
        in = insert_cti_after(
            ls, in,
            EVENT_ENTER_MODULE, false, operand(),
            CTI_CALL);
        in.set_mangled();
    }


    /// Instrument a return to native code.
    static void instrument_return_to_native(
        granary::instruction_list &ls,
        granary::instruction in
    ) throw() {
        using namespace granary;

        in = ls.insert_before(in, label_());
        in = insert_cti_after(
            ls, in,
            EVENT_EXIT_MODULE, false, operand(),
            CTI_CALL);
        in.set_mangled();
    }


    /// Instruction a basic block.
    granary::instrumentation_policy entry_block_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &ls
    ) throw() {
        using namespace granary;

        instruction in(ls.first());
        for(; in.is_valid(); in = in.next()) {
            if(in.is_call()) {
                in.set_policy(policy_for<internal_code_policy>());
            } else if(in.is_return()) {
                instrument_return_to_native(ls, in);
            }
        }

        instrument_entry_to_code_cache(ls);

        return granary::policy_for<entry_code_policy>();
    }


    /// Keeps us in the entry code policy until a CALL brings us to the
    /// `internal_code_policy` policy.
    granary::instrumentation_policy entry_code_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &ls
    ) throw() {
        using namespace granary;

        instruction in(ls.first());
        for(; in.is_valid(); in = in.next()) {
            if(in.is_call()) {
                in.set_policy(policy_for<internal_code_policy>());
            } else if(in.is_return()) {
                instrument_return_to_native(ls, in);
            }
        }

        return granary::policy_for<entry_code_policy>();
    }


    /// Keeps us in the internal code policy until a RET brings us to another
    /// policy.
    granary::instrumentation_policy internal_code_policy::visit_app_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<internal_code_policy>();
    }


    /// Instrument a basic block of host code.
    granary::instrumentation_policy entry_block_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<entry_block_policy>();
    }


    granary::instrumentation_policy entry_code_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<entry_code_policy>();
    }


    granary::instrumentation_policy internal_code_policy::visit_host_instructions(
        granary::cpu_state_handle,
        granary::basic_block_state &,
        granary::instruction_list &
    ) throw() {
        return granary::policy_for<internal_code_policy>();
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state entry_block_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }


    granary::interrupt_handled_state entry_code_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }


    granary::interrupt_handled_state internal_code_policy::handle_interrupt(
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




#if CONFIG_RUN_TEST_CASES

namespace test {


    DONT_OPTIMISE static void func_b(void) throw() {
        ASM("");
    }


    DONT_OPTIMISE static void func_a(void) throw() {
        func_b();
        func_b();
    }


    /// Test jcxz/jecxz/jrcxz; Note: there is no far version of jrxcz.
    static void test_policy_switching(void) {
        granary::app_pc func_a_addr((granary::app_pc) func_a);
        granary::basic_block func_a_bb(granary::code_cache::find(
            func_a_addr, granary::policy_for<client::entry_block_policy>()));

        func_a_bb.call<void>();
    }


    ADD_TEST(test_policy_switching,
        "Test that policy switching works correctly.")
}

#endif /* CONFIG_RUN_TEST_CASES */
