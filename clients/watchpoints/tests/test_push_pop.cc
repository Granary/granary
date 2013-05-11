/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_push_pop.cc
 *
 *  Created on: Apr 30, 2013
 *      Author: Peter Goodman
 */

#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

#include "clients/watchpoints/policies/null_policy.h"
#include "clients/watchpoints/tests/pp.h"

namespace test {

#if GRANARY_IN_KERNEL
#   define MASK_OP "and"
#else
#   define MASK_OP "or"
#endif

    extern "C" {
        uint64_t WP_PP_FOO = 0;
        uint64_t WP_PP_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }

    static uint64_t unwatched_push(void) throw() {
        register uint64_t ret(0);
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_PP_FOO)) ", %%rax;"
            "pushq (%%rax);"
            "popq %%rax;"
            "movq %%rax, %0;"
            : "=r"(ret)
            :
            : "%rax", "%rbx"
        );
        return ret;
    }

    static uint64_t watched_push(void) throw() {
        register uint64_t ret(0);
        ASM(
            "movq " TO_STRING(SYMBOL(WP_PP_MASK)) ", %%rax;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_PP_FOO)) ", %%rax;" // mask the address of FOO
            "pushq (%%rax);"
            "popq %%rax;"
            "movq %%rax, %0;"
            : "=r"(ret)
            :
            : "%rax", "%rbx"
        );
        return ret;
    }


    static void unwatched_pop(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_PP_FOO)) ", %%rax;"
            "movq $0xDEADBEEF, %%rbx;"
            "pushq %%rbx;"
            "popq (%%rax);"
            :
            :
            : "%rax", "%rbx"
        );
    }


    static void watched_pop(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_PP_MASK)) ", %%rax;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_PP_FOO)) ", %%rax;" // mask the address of FOO
            "movq $0xDEADBEEF, %%rbx;"
            "pushq %%rbx;"
            "popq (%%rax);"
            :
            :
            : "%rax", "%rbx"
        );
    }


    /// Test that PUSH and POP instructions are correctly watched.
    static void push_pop_watched_correctly(void) {
        (void) WP_PP_FOO;
        (void) WP_PP_MASK;

        granary::app_pc push((granary::app_pc) unwatched_push);
        granary::basic_block call_push(granary::code_cache::find(
                push, granary::policy_for<client::watchpoint_null_policy>()));

        WP_PP_FOO = 0xDEADBEEF;
        ASSERT(0xDEADBEEF == call_push.call<uint64_t>());

        granary::app_pc wpush((granary::app_pc) watched_push);
        granary::basic_block call_wpush(granary::code_cache::find(
            wpush, granary::policy_for<client::watchpoint_null_policy>()));

        WP_PP_FOO = 0xBEEFDEAD;
        ASSERT(0xBEEFDEAD == call_wpush.call<uint64_t>());


        granary::app_pc pop((granary::app_pc) unwatched_pop);
        granary::basic_block call_pop(granary::code_cache::find(
            pop, granary::policy_for<client::watchpoint_null_policy>()));

        WP_PP_FOO = 0;
        call_pop.call<void>();
        ASSERT(0xDEADBEEF == WP_PP_FOO);

        granary::app_pc wpop((granary::app_pc) watched_pop);
        granary::basic_block call_wpop(granary::code_cache::find(
            wpop, granary::policy_for<client::watchpoint_null_policy>()));

        WP_PP_FOO = 0;
        call_wpop.call<void>();
        ASSERT(0xDEADBEEF == WP_PP_FOO);

    }

    ADD_TEST(push_pop_watched_correctly,
        "Test that PUSH and POP instructions are correctly watched.")
}

#endif /* CONFIG_RUN_TEST_CASES */
