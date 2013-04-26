/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test.h
 *
 *      Author: Peter Goodman
 */

#ifndef granary_TEST_H_
#define granary_TEST_H_

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/state.h"
#include "granary/emit_utils.h"

#if CONFIG_RUN_TEST_CASES
#   include "granary/x86/asm_defines.asm"
#   include "granary/detach.h"
#endif

namespace granary {

    /// Instrumentation policy for basic blocks using tests.
    struct test_policy : public instrumentation_policy {
    public:

        /// Instruction a basic block.
        static instrumentation_policy visit_basic_block(
            cpu_state_handle &,
            thread_state_handle &,
            basic_block_state &,
            instruction_list &
        ) throw();


#if CONFIG_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in module code. Returns true iff the client
        /// handles the interrupt.
        static bool handle_interrupt(
            cpu_state_handle &cpu,
            thread_state_handle &thread,
            basic_block_state &bb,
            interrupt_stack_frame &isf,
            interrupt_vector vector
        ) throw();
#endif

    };

#if CONFIG_RUN_TEST_CASES
    /// Used for static initialisation of test cases.
    struct static_test_list {
        void (*func)(void);
        const char *desc;
        static_test_list *next;

        static_test_list(void) throw();

        static void append(static_test_list &) throw();
    };

    void run_tests(void) throw();
#endif /* CONFIG_RUN_TEST_CASES */

}


#endif /* granary_TEST_H_ */
