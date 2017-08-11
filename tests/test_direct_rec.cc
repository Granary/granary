/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_direct_timing.cc
 *
 *  Created on: 2013-01-25
 *      Author: pag
 */

#include "granary/test.h"

#if CONFIG_DEBUG_RUN_TEST_CASES

namespace test {


    static int test_fibonacci(int i) {
        if(0 == i) {
            return 1;
        } else if(1 == i) {
            return 1;
        } else {
            return test_fibonacci(i - 1) + test_fibonacci(i - 2);
        }
    }


    /// Test the overhead of a native Fibonacci call and a null instrumented
    /// Fibonacci call.
    static void direct_rec_fibonacci(void) {
        int native_result;
        int inst_result;

        granary::app_pc fib((granary::app_pc) test_fibonacci);
        granary::basic_block bb_fib(granary::code_cache::find(
            fib, granary::TEST_POLICY));

        native_result = test_fibonacci(5);
        inst_result = bb_fib.call<int, int>(5);

        ASSERT(native_result == inst_result);
    }


    ADD_TEST(direct_rec_fibonacci,
        "Test that the Fibonacci function is correctly computed using a naive "
        "recursive formulation.")
}

#endif

