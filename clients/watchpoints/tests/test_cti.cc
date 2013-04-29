/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_cti.cc
 *
 *  Created on: 2013-04-28
 *      Author: Peter Goodman
 */

#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

#include "clients/watchpoints/policies/null_policy.h"
#include "clients/watchpoints/tests/pp.h"

namespace test {

#if GRANARY_IN_KERNEL
#   define MASK_OP &=
#else
#   define MASK_OP |=
#endif

    struct thunk {
        int (*func)(void);
    };

    static bool indirect_call(thunk *thunk) throw() {
        bool ret(thunk->func());
        ASM("");
        return ret;
    }

    /// Test that the targets of indirect calls are correctly resolved.
    static void indirect_cti_watched_correctly(void) {
        granary::app_pc call((granary::app_pc) indirect_call);
        granary::basic_block call_call(granary::code_cache::find(
            call, granary::policy_for<client::watchpoint_null_policy>()));

        thunk foo;
        foo.func = &granary_test_return_true;
        ASSERT(call_call.call<bool, thunk *>(&foo));

        union {
            thunk *foo;
            uint64_t address;
        } wfoo;
        wfoo.foo = &foo;
        wfoo.address MASK_OP client::wp::DISTINGUISHING_BIT_MASK;
        ASSERT(call_call.call<bool, thunk *>(wfoo.foo));
    }


    ADD_TEST(indirect_cti_watched_correctly,
        "Test that indirect CTIs are watched correctly.")
}

#endif /* CONFIG_RUN_TEST_CASES */
