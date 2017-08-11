/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_xlat.cc
 *
 *  Created on: 2013-04-28
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
        uint8_t WP_XLAT_TABLE[256] = {0};
        uint64_t WP_XLAT_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }

    static uint64_t unwatched_xlat(register uint64_t val) {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_XLAT_TABLE)) ", %%rbx;"
            "movq %1, %%rax;"
            "xlat;"
            "movq %%rax, %0"
            : "=r"(val)
            : "r"(val)
            : "%rax", "%rbx"
        );
        return val;
    }

    static uint64_t watched_xlat(register uint64_t val) {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_XLAT_MASK)) ", %%rbx;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_XLAT_TABLE)) ", %%rbx;"
            "movq %1, %%rax;"
            "xlat;"
            "movq %%rax, %0"
            : "=r"(val)
            : "r"(val)
            : "%rax", "%rbx"
        );
        return val;
    }

    static uint64_t unwatched_xlat_rbx_live(register uint64_t val) {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_XLAT_TABLE)) ", %%rbx;"
            "movq %1, %%rax;"

            // save value of RBX before it's modified by the instrumentation.
            "movq %%rbx, %%rcx;"

            "xlat;" // instrumented

            // check and ensure that RBX is correctly restored after the XLAT
            // instruction by comparing against RCX.
            "cmp %%rbx, %%rcx;"
            "movq $1, %%rcx;"
            "cmovne %%rcx, %%rax;"

            "movq %%rax, %0" // output

            : "=r"(val)
            : "r"(val)
            : "%rax", "%rbx", "%rcx"
        );
        return val;
    }

    static uint64_t watched_xlat_rbx_live(register uint64_t val) {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_XLAT_MASK)) ", %%rbx;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_XLAT_TABLE)) ", %%rbx;"
            "movq %1, %%rax;"
            // save value of RBX before it's modified by the instrumentation.
            "movq %%rbx, %%rcx;"

            "xlat;"

            // check and ensure that RBX is correctly restored after the XLAT
            // instruction by comparing against RCX.
            "cmp %%rbx, %%rcx;"
            "movq $1, %%rcx;"
            "cmovne %%rcx, %%rax;"

            "movq %%rax, %0"

            : "=r"(val)
            : "r"(val)
            : "%rax", "%rbx", "%rcx"
        );
        return val;
    }

    /// Test that XLAT instructions are correctly watched.
    static void xlat_watched_correctly(void) {
        (void) WP_XLAT_TABLE;
        (void) WP_XLAT_MASK;

        // Simple un/watched, no flags dependencies, no register dependencies.

        granary::app_pc xlat((granary::app_pc) unwatched_xlat);
        granary::basic_block call_xlat(granary::code_cache::find(
            xlat, granary::policy_for<client::watchpoint_null_policy>()));

        for(uint64_t i(0); i < 256; ++i) {
            ASSERT(0 == call_xlat.call<uint64_t, uint64_t>(i));
        }

        granary::app_pc wxlat((granary::app_pc) watched_xlat);
        granary::basic_block call_wxlat(granary::code_cache::find(
            wxlat, granary::policy_for<client::watchpoint_null_policy>()));

        for(uint64_t i(0); i < 256; ++i) {
            ASSERT(0 == call_wxlat.call<uint64_t, uint64_t>(i));
        }

        // Simple un/watched, no flags dependencies, but RBX is live after the
        // XLAT. The test will fail if RBX is not restored to its value before
        // the XLAT is executed.

        granary::app_pc xlat_rbx((granary::app_pc) unwatched_xlat_rbx_live);
        granary::basic_block call_xlat_rbx(granary::code_cache::find(
            xlat_rbx, granary::policy_for<client::watchpoint_null_policy>()));

        for(uint64_t i(0); i < 256; ++i) {
            ASSERT(0 == call_xlat_rbx.call<uint64_t, uint64_t>(i));
        }

        granary::app_pc wxlat_rbx((granary::app_pc) watched_xlat_rbx_live);
        granary::basic_block call_wxlat_rbx(granary::code_cache::find(
            wxlat_rbx, granary::policy_for<client::watchpoint_null_policy>()));

        for(uint64_t i(0); i < 256; ++i) {
            ASSERT(0 == call_wxlat_rbx.call<uint64_t, uint64_t>(i));
        }
    }


    ADD_TEST(xlat_watched_correctly,
        "Test that XLAT instructions are correctly watched.")
}

#endif /* CONFIG_DEBUG_RUN_TEST_CASES */
