/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/test.h"
#include "granary/register.h"

extern "C" {


#if CONFIG_RUN_TEST_CASES
    DONT_OPTIMISE int granary_test_return_true(void) {
        return 1;
    }

    DONT_OPTIMISE int granary_test_return_false(void) {
        return 0;
    }
#endif

#if !GRANARY_IN_KERNEL

    DONT_OPTIMISE void granary_break_on_fault(void) {
        ASM("");
    }


    DONT_OPTIMISE void granary_break_on_curiosity(void) {
        ASM("");
    }


    DONT_OPTIMISE int granary_fault(void) {
        ASM("int3; int3; mov 0, %rax;");
        return 1;
    }

#endif

}

namespace granary {


    /// Instruction a basic block.
    instrumentation_policy test_policy::visit_app_instructions(
        cpu_state_handle,
        basic_block_state &,
        instruction_list &
    ) throw() {
        return granary::policy_for<test_policy>();
    }


    /// Instruction a basic block.
    instrumentation_policy test_policy::visit_host_instructions(
        cpu_state_handle,
        basic_block_state &,
        instruction_list &
    ) throw() {
        return granary::policy_for<test_policy>();
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state test_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw() {
        return granary::INTERRUPT_DEFER;
    }

#endif


#if CONFIG_RUN_TEST_CASES

    /// List of test cases to run.
    static static_test_list STATIC_TEST_LIST_HEAD;


    /// Initialise the list of test functions to execute.
    static_test_list::static_test_list(void) throw()
        : func(nullptr)
        , desc(nullptr)
        , next(nullptr)
    { }


    void static_test_list::append(static_test_list &entry) throw() {
        entry.next = STATIC_TEST_LIST_HEAD.next;
        STATIC_TEST_LIST_HEAD.next = &entry;
    }


    void run_tests(void) throw() {

        static_test_list *test(STATIC_TEST_LIST_HEAD.next);
        for(; test; test = test->next) {
            if(test->func) {
                IF_KERNEL( eflags flags = granary_disable_interrupts(); )
                cpu_state_handle cpu;
                IF_TEST( cpu->in_granary = false; )
                cpu.free_transient_allocators();
                printf("Running test '%s'\n", test->desc);
                test->func();
                IF_KERNEL( granary_store_flags(flags); )
            }
        }
    }
#endif /* CONFIG_RUN_TEST_CASES */
} /* granary */

