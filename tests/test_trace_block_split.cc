/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_trace_block_split.cc
 *
 *  Created on: 2013-11-18
 *      Author: Peter Goodman
 */

#include "granary/test.h"

#if CONFIG_DEBUG_RUN_TEST_CASES && CONFIG_FOLLOW_FALL_THROUGH_BRANCHES

namespace test {


    /// A test of trace basic block splitting. Part of what we care about here
    /// is that the ordering of basic blocks in the code cache is reasonable
    /// (which requires manual inspection)
    static void block_split_order(void) {
        ASM(
            "mov $1, %rax;"
            "test %rax, %rax;" // make sure not zero
        "1:"
            "mov $2, %rax;"
        "2:"
            "mov $3, %rax;"
            "jz 1b;"
            "jnz 1f;"
            "jmp 2b;"
        "1:"
            "mov $4, %rax;"
        );
    }


    static void test_block_split(void) {
        granary::app_pc func((granary::app_pc) block_split_order);
        granary::basic_block bb_func(granary::code_cache::find(
            func, granary::TEST_POLICY));
        bb_func.call<void>();
    }


    ADD_TEST(test_block_split,
        "Test that basic block splitting in traces works.")

}

#endif


