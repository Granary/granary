/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_cmp.cc
 *
 *  Created on: 2013-10-13
 *      Author: Peter Goodman
 */


#include "granary/test.h"

#if CONFIG_DEBUG_RUN_TEST_CASES

#include "clients/watchpoints/clients/null/instrument.h"
#include "clients/watchpoints/clients/null/tests/pp.h"

namespace test {

#if CONFIG_ENV_KERNEL
#   define MASK_OP "andq"
#else
#   define MASK_OP "orq"
#endif

    extern "C" {
        uint64_t WP_CMP_VAL = 0;
        uint64_t WP_CMP_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }

    static int watched_cmp_through_self(void) throw() {
        int64_t ret = 0;
        ASM(
            "call 1f;"
            "movq %%rax, %0;"
            "jmp .done_cmp_test;"
            "1:"
            "movq " TO_STRING(SYMBOL(WP_CMP_MASK)) ", %%rbx;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_CMP_VAL)) ", %%rbx;"
            "movq %%rbx, (%%rbx);"
            "cmpq %%rbx, (%%rbx);"
            "je 1f;"
            "movq $0, %%rax; ret;"
            "1: movq $1, %%rax; ret;"
            ".done_cmp_test:"
            : "=m"(ret)
            :
            : "%rax", "%rbx"
        );
        return ret;
    }


    /// Test that CMP instructions are correctly watched. Specifically, we want
    /// to test the case of `CMP %rbx, (%rbx)` where `%rbx` is watched and
    /// `(%rbx)` contains the watched `%rbx`.
    static void cmp_watched_correctly(void) {
        (void) WP_CMP_VAL;
        (void) WP_CMP_MASK;

        granary::instrumentation_policy policy =
            granary::policy_for<client::watchpoint_null_policy>();
        policy.force_attach(true);

        granary::app_pc cmp((granary::app_pc) watched_cmp_through_self);
        granary::basic_block call_cmp(granary::code_cache::find(
            cmp, policy));

        ASSERT(1 == call_cmp.call<int>());
    }


    ADD_TEST(cmp_watched_correctly,
        "Test that CMP instructions are correctly watched.")
}

#endif /* CONFIG_DEBUG_RUN_TEST_CASES */


