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
#include "granary/x86/asm_helpers.asm"

extern "C" {


#if CONFIG_DEBUG_RUN_TEST_CASES
    DONT_OPTIMISE int granary_test_return_true(void) {
        return 1;
    }

    DONT_OPTIMISE int granary_test_return_false(void) {
        return 0;
    }
#endif

#if !CONFIG_ENV_KERNEL

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
    ) {
        return granary::policy_for<test_policy>();
    }


    /// Instruction a basic block.
    instrumentation_policy test_policy::visit_host_instructions(
        cpu_state_handle,
        basic_block_state &,
        instruction_list &
    ) {
        return granary::policy_for<test_policy>();
    }


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT

    /// Handle an interrupt in module code. Returns true iff the client
    /// handles the interrupt.
    granary::interrupt_handled_state test_policy::handle_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::basic_block_state &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) {
        return granary::INTERRUPT_DEFER;
    }

#endif


    /// List of test cases to run.
    static static_test_list STATIC_TEST_LIST_HEAD;


    /// Initialise the list of test functions to execute.
    static_test_list::static_test_list(void) 
        : func(nullptr)
        , desc(nullptr)
        , next(nullptr)
    { }


    void static_test_list::append(static_test_list &entry) {
        entry.next = STATIC_TEST_LIST_HEAD.next;
        STATIC_TEST_LIST_HEAD.next = &entry;
    }


    extern "C" {

        void granary_do_test_on_private_stack(static_test_list *test) {
            test->func();
        }


        extern void granary_enter_private_stack(void);
        extern void granary_exit_private_stack(void);
    }


    instrumentation_policy TEST_POLICY;


    void run_tests(void) {

        TEST_POLICY = granary::policy_for<granary::test_policy>();
        TEST_POLICY.force_attach(true);
        TEST_POLICY.return_address_in_code_cache(true);
        TEST_POLICY.begins_functional_unit(true);
        TEST_POLICY.in_host_context(false);

        static_test_list *test(STATIC_TEST_LIST_HEAD.next);
        for(; test; test = test->next) {
            if(test->func) {
                printf("Running test '%s'\n", test->desc);

                IF_KERNEL( eflags flags = granary_disable_interrupts(); )
                cpu_state_handle cpu;
                IF_TEST( cpu->in_granary = false; )
                enter(cpu);

                ASM(
                    "movq %0, %%" TO_STRING(ARG1) ";"
                    TO_STRING(PUSHA_ASM_ARG)
                    IF_KERNEL( "callq " TO_STRING(SHARED_SYMBOL(granary_enter_private_stack)) ";" )
                    "callq " TO_STRING(SHARED_SYMBOL(granary_do_test_on_private_stack)) ";"
                    IF_KERNEL( "callq " TO_STRING(SHARED_SYMBOL(granary_exit_private_stack)) ";" )
                    TO_STRING(POPA_ASM_ARG)
                    :
                    : "m"(test)
                    : "%" TO_STRING(ARG1)
                );

                IF_KERNEL( granary_store_flags(flags); )
            }
        }
    }
} /* granary */

