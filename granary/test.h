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

#include "granary/x86/asm_defines.asm"
#include "granary/detach.h"

namespace granary {


    extern instrumentation_policy TEST_POLICY;


    /// Instrumentation policy for basic blocks using tests.
    struct test_policy : public instrumentation_policy {
    public:


        enum {
            AUTO_INSTRUMENT_HOST = false
        };


        /// Instruction a basic block.
        instrumentation_policy visit_app_instructions(
            cpu_state_handle,
            basic_block_state &,
            instruction_list &
        ) throw();


        /// Instruction a basic block.
        instrumentation_policy visit_host_instructions(
            cpu_state_handle,
            basic_block_state &,
            instruction_list &
        ) throw();


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in module code. Returns true iff the client
        /// handles the interrupt.
        granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle cpu,
            granary::thread_state_handle thread,
            granary::basic_block_state &bb,
            granary::interrupt_stack_frame &isf,
            granary::interrupt_vector vector
        ) throw();
#endif

    };

    /// Used for static initialisation of test cases.
    struct static_test_list {
        void (*func)(void);
        const char *desc;
        static_test_list *next;

        static_test_list(void) throw();

        static void append(static_test_list &) throw();
    };

    void run_tests(void) throw();

}


#endif /* granary_TEST_H_ */
