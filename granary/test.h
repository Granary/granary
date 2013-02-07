/*
 * test.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_TEST_H_
#define granary_TEST_H_

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/code_cache.h"
#include "granary/basic_block.h"
#include "granary/state.h"
#include "granary/x86/asm_defines.asm"
#include "granary/detach.h"
#include "granary/emit_utils.h"

namespace granary {


    /// Instrumentation policy for basic blocks using tests.
    struct test_policy {
    public:

        /// Instruction a basic block.
        static instrumentation_policy visit_basic_block(
            cpu_state_handle &,
            thread_state_handle &,
            basic_block_state &,
            instruction_list &
        ) throw();
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
