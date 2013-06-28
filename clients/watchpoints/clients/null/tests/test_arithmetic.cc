/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_arithmetic.cc
 *
 *  Created on: 2013-04-28
 *      Author: Peter Goodman
 */

#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

#include "clients/watchpoints/clients/null/instrument.h"
#include "clients/watchpoints/clients/null/tests/pp.h"

namespace test {

#if GRANARY_IN_KERNEL
#   define MASK_OP "andq"
#else
#   define MASK_OP "orq"
#endif

    extern "C" {
        uint64_t WP_ARITH_FOO = 0;
        uint64_t WP_ARITH_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }

    static void unwatched_add(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_ARITH_FOO)) ", %rax;"
            "movq $1, %rbx;"
            "add %rbx, (%rax);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    static void watched_add(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_ARITH_MASK)) ", %rax;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_ARITH_FOO)) ", %rax;" // mask the address of FOO
            "movq $1, %rbx;"
            "add %rbx, (%rax);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    static void unwatched_xadd(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_ARITH_FOO)) ", %rax;"
            "movq $1, %rbx;"
            "xadd %rbx, (%rax);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    static void watched_xadd(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_ARITH_MASK)) ", %rax;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_ARITH_FOO)) ", %rax;" // mask the address of FOO
            "movq $1, %rbx;"
            "xadd %rbx, (%rax);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    static void unwatched_inc(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_ARITH_FOO)) ", %rax;"
            "incq (%rax);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    static void watched_inc(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_ARITH_MASK)) ", %rax;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_ARITH_FOO)) ", %rax;" // mask the address of FOO
            "incq (%rax);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    /// Test that arithmetic instructions are correctly watched.
    static void arithmetic_watched_correctly(void) {
        (void) WP_ARITH_FOO;
        (void) WP_ARITH_MASK;

        // Simple un/watched, no flags dependencies, no register dependencies.

        granary::app_pc add((granary::app_pc) unwatched_add);
        granary::basic_block call_add(granary::code_cache::find(
            add, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ARITH_FOO = 0;
        call_add.call<void>();
        ASSERT(1 == WP_ARITH_FOO);

        granary::app_pc wadd((granary::app_pc) watched_add);
        granary::basic_block call_wadd(granary::code_cache::find(
            wadd, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ARITH_FOO = 0;
        call_wadd.call<void>();
        ASSERT(1 == WP_ARITH_FOO);

        granary::app_pc xadd((granary::app_pc) unwatched_xadd);
        granary::basic_block call_xadd(granary::code_cache::find(
            xadd, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ARITH_FOO = 0;
        call_xadd.call<void>();
        ASSERT(1 == WP_ARITH_FOO);

        granary::app_pc wxadd((granary::app_pc) watched_xadd);
        granary::basic_block call_wxadd(granary::code_cache::find(
            wxadd, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ARITH_FOO = 0;
        call_wxadd.call<void>();
        ASSERT(1 == WP_ARITH_FOO);

        granary::app_pc inc((granary::app_pc) unwatched_inc);
        granary::basic_block call_inc(granary::code_cache::find(
            inc, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ARITH_FOO = 0;
        call_inc.call<void>();
        ASSERT(1 == WP_ARITH_FOO);

        granary::app_pc winc((granary::app_pc) unwatched_inc);
        granary::basic_block call_winc(granary::code_cache::find(
            winc, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ARITH_FOO = 0;
        call_winc.call<void>();
        ASSERT(1 == WP_ARITH_FOO);
    }


    ADD_TEST(arithmetic_watched_correctly,
        "Test that arithmetic instructions are correctly watched.")
}

#endif /* CONFIG_RUN_TEST_CASES */
