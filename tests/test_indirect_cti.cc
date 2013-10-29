/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_indirect_cti.cc
 *
 *  Created on: 2013-01-29
 *      Author: pag
 */


#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

namespace test {


    static int (*true_func)(void) = granary_test_return_true;


    static int (*false_func)(void) = granary_test_return_false;


    static bool indirect_call(void) throw() {
        if(true_func() && false_func()) {
            return false;
        }
        return true;
    }


    /// Test that the targets of indirect calls are correctly resolved.
    static void indirect_call_mangled_correctly(void) {
        granary::app_pc indirect_call_pc((granary::app_pc) indirect_call);

        granary::basic_block bb_indirect_call(granary::code_cache::find(
            indirect_call_pc, granary::TEST_POLICY));

        // call it a few times, ideally to exercise both slow path (global
        // lookup) and for the fast path (local lookup).
        for(int i(0); i < 10; ++i) {
            ASSERT(bb_indirect_call.call<bool>());
            ASSERT(bb_indirect_call.call<bool>());
        }
    }


    ADD_TEST(indirect_call_mangled_correctly,
        "Test that the targets of indirect calls are correctly resolved.")
}

#endif
