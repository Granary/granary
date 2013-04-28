/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_string.cc
 *
 *  Created on: 2013-04-28
 *      Author: Peter Goodman
 */

#include "granary/test.h"

#include "clients/watchpoints/policies/null_policy.h"
#include "clients/watchpoints/tests/pp.h"

namespace test {

#if GRANARY_IN_KERNEL
#   define MASK_OP "andq"
#else
#   define MASK_OP "orq"
#endif

    extern "C" {
        uint64_t WP_STRING_FOO = 0;
        uint64_t WP_STRING_FOO_ARRAY[2] = {0};
        uint64_t WP_STRING_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }


    static void unwatched_stos(void) throw() {
        ASM(
            "movq $WP_STRING_FOO, %rdi;"
            "movq $1, %rax;"
            "stosq;"
        );
    }


    static void watched_stos(void) throw() {
        ASM(
            "movq WP_STRING_MASK, %rdi;"
            MASK_OP " $WP_STRING_FOO, %rdi;" // mask the address of FOO

            "movq $1, %rax;"
            "stosq;"
        );
    }


    static void unwatched_stos_dep(void) throw() {
        ASM(
            "movq $WP_STRING_FOO, %rdi;"
            "movq $1, %rax;"
            "stosq;"

            PUSHA POPA
        );
    }


    static void watched_stos_dep(void) throw() {
        ASM(
            "movq WP_STRING_MASK, %rdi;"
            MASK_OP " $WP_STRING_FOO, %rdi;" // mask the address of FOO

            "movq $1, %rax;"
            "stosq;"

            PUSHA POPA
        );
    }


    /* REP STOS m64         Fill RCX quadwords at [RDI] with RAX. */


    static void unwatched_rep_stos(void) throw() {
        ASM(
            "movq $WP_STRING_FOO_ARRAY, %rdi;"
            "movq $2, %rcx;"
            "movq $1, %rax;"
            "rep stosq;"
        );
    }


    static void watched_rep_stos(void) throw() {
        ASM(
            "movq WP_STRING_MASK, %rdi;"
            MASK_OP " $WP_STRING_FOO_ARRAY, %rdi;" // mask the address of FOO

            "movq $2, %rcx;"
            "movq $1, %rax;"
            "rep stosq;"
        );
    }


    static void unwatched_rep_stos_dep(void) throw() {
        ASM(
            "movq $WP_STRING_FOO_ARRAY, %rdi;"
            "movq $2, %rcx;"
            "movq $1, %rax;"
            "rep stosq;"

            PUSHA POPA
        );
    }


    static void watched_rep_stos_dep(void) throw() {
        ASM(
            "movq WP_STRING_MASK, %rdi;"
            MASK_OP " $WP_STRING_FOO_ARRAY, %rdi;" // mask the address of FOO

            "movq $2, %rcx;"
            "movq $1, %rax;"
            "rep stosq;"

            PUSHA POPA
        );
    }


    /// Test that arithmetic instructions are correctly watched.
    static void string_watched_correctly(void) {
        (void) WP_STRING_MASK;

        granary::app_pc stos((granary::app_pc) unwatched_stos);
        granary::basic_block call_stos(granary::code_cache::find(
            stos, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO = 0;
        call_stos.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        granary::app_pc wstos((granary::app_pc) watched_stos);
        granary::basic_block call_wstos(granary::code_cache::find(
            wstos, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO = 0;
        call_wstos.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        granary::app_pc stos_dep((granary::app_pc) unwatched_stos_dep);
        granary::basic_block call_stos_dep(granary::code_cache::find(
            stos_dep, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO = 0;
        call_stos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        granary::app_pc wstos_dep((granary::app_pc) watched_stos_dep);
        granary::basic_block call_wstos_dep(granary::code_cache::find(
            wstos_dep, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO = 0;
        call_wstos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        // With REP prefix:

        granary::app_pc rep_stos((granary::app_pc) unwatched_rep_stos);
        granary::basic_block call_rep_stos(granary::code_cache::find(
                rep_stos, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_stos.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);

        granary::app_pc rep_wstos((granary::app_pc) watched_rep_stos);
        granary::basic_block call_rep_wstos(granary::code_cache::find(
            rep_wstos, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_wstos.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);

        granary::app_pc rep_stos_dep((granary::app_pc) unwatched_rep_stos_dep);
        granary::basic_block call_rep_stos_dep(granary::code_cache::find(
            rep_stos_dep, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_stos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);

        granary::app_pc rep_wstos_dep((granary::app_pc) watched_rep_stos_dep);
        granary::basic_block call_rep_wstos_dep(granary::code_cache::find(
            rep_wstos_dep, granary::policy_for<client::watchpoint_null_policy>()));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_wstos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);
    }

    ADD_TEST(string_watched_correctly,
        "Test that string instructions are correctly watched.")
}


