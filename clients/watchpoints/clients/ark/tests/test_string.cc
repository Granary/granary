/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_string.cc
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
        uint64_t WP_STRING_FOO = 0;
        uint64_t WP_STRING_FOO_ARRAY[2] = {0};
        uint64_t WP_STRING_FOO_ARRAY_DEST[2] = {0};
        uint64_t WP_STRING_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }


    static void unwatched_stos(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO)) ", %rdi;"
            "movq $1, %rax;"

            // next value of %rdi after the STOS
            "movq %rdi, %rdx;"
            "addq $8, %rdx;"

            "cld;"
            "stosq;"

            // test that the address in RDI is correctly updated
            "cmpq %rdi, %rdx;"
            "je 1f;"
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO)) ", %rdx;"
            "xor %rax, %rax;"
            "movq %rax, (%rdx);"
            "1: nop;"
        );
    }


    static void watched_stos(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_STRING_MASK)) ", %rdi;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_STRING_FOO)) ", %rdi;" // mask the address of FOO
            "movq $1, %rax;"

            // next value of %rdi after the STOS
            "movq %rdi, %rdx;"
            "addq $8, %rdx;"

            "cld;"
            "stosq;"

            // test that the address in RDI is correctly updated, and maintains
            // its watchpoint
            "cmpq %rdi, %rdx;"
            "je 1f;"
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO)) ", %rdx;"
            "xor %rax, %rax;"
            "movq %rax, (%rdx);"
            "1: nop;"
        );
    }


    static void unwatched_stos_dep(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO)) ", %rdi;"
            "movq $1, %rax;"
            "cld;"
            "stosq;"

            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }


    static void watched_stos_dep(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_STRING_MASK)) ", %rdi;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_STRING_FOO)) ", %rdi;" // mask the address of FOO

            "movq $1, %rax;"
            "cld;"
            "stosq;"

            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }


    /* REP STOS m64         Fill RCX quadwords at [RDI] with RAX. */


    static void unwatched_rep_stos(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY)) ", %rdi;"
            "movq $2, %rcx;"
            "movq $1, %rax;"
            "cld;"
            "rep stosq;"
        );
    }


    static void watched_rep_stos(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_STRING_MASK)) ", %rdi;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY)) ", %rdi;" // mask the address of FOO

            "movq $2, %rcx;"
            "movq $1, %rax;"
            "cld;"
            "rep stosq;"
        );
    }


    static void unwatched_rep_stos_dep(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY)) ", %rdi;"
            "movq $2, %rcx;"
            "movq $1, %rax;"
            "cld;"
            "rep stosq;"

            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }


    static void watched_rep_stos_dep(void) throw() {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_STRING_MASK)) ", %rdi;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY)) ", %rdi;" // mask the address of FOO

            "movq $2, %rcx;"
            "movq $1, %rax;"
            "cld;"
            "rep stosq;"

            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }


    /* REP MOVS m64, m64   Move RCX quadwords from [RSI] to [RDI].*/

    static void unwatched_rep_movs(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY)) ", %rsi;"
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY_DEST)) ", %rdi;"
            "movq $2, %rcx;"
            "cld;"
            "rep movsq %ds:(%rsi),%es:(%rdi);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }


    // modification of code from memmove$VARIANT$sse3x so as to maintain
    // the same dependencies, but not have any meaningful side-effects.
    static void unwatched_rep_movs_memmove(void) throw() {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY)) ", %rsi;"
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO_ARRAY_DEST)) ", %rdi;"
            "movq $2, %rcx;"
            "cld;"
            "rep movsq %ds:(%rsi),%es:(%rdi);"

            "mov %rdx,%rcx;"
            "mov %esi,%eax;"
            "and $0x3f,%edx;"
            "and $0xf,%eax;"
            "and $-64,%rcx;"
            "movq $" TO_STRING(SYMBOL(WP_STRING_FOO)) ",%r8;"
            "add %rcx,%rsi;"
            "add %rcx,%rsi;"
            "add %rcx,%rdi;"
            "mov $0, %rax;"
            "mov (%r8,%rax,4),%eax;"
            "neg %rcx;"
            "add %r8,%rax;"

            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }


    /// Test that arithmetic instructions are correctly watched.
    static void string_watched_correctly(void) {
        (void) WP_STRING_MASK;

        granary::instrumentation_policy policy =
            granary::policy_for<client::watchpoint_null_policy>();
        policy.force_attach(true);

        granary::app_pc stos((granary::app_pc) unwatched_stos);
        granary::basic_block call_stos(granary::code_cache::find(
            stos, policy));

        WP_STRING_FOO = 0;
        call_stos.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        granary::app_pc wstos((granary::app_pc) watched_stos);
        granary::basic_block call_wstos(granary::code_cache::find(
            wstos, policy));

        WP_STRING_FOO = 0;
        call_wstos.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        granary::app_pc stos_dep((granary::app_pc) unwatched_stos_dep);
        granary::basic_block call_stos_dep(granary::code_cache::find(
            stos_dep, policy));

        WP_STRING_FOO = 0;
        call_stos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        granary::app_pc wstos_dep((granary::app_pc) watched_stos_dep);
        granary::basic_block call_wstos_dep(granary::code_cache::find(
            wstos_dep, policy));

        WP_STRING_FOO = 0;
        call_wstos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO);

        // With REP prefix:

        granary::app_pc rep_stos((granary::app_pc) unwatched_rep_stos);
        granary::basic_block call_rep_stos(granary::code_cache::find(
                rep_stos, policy));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_stos.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);

        granary::app_pc rep_wstos((granary::app_pc) watched_rep_stos);
        granary::basic_block call_rep_wstos(granary::code_cache::find(
            rep_wstos, policy));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_wstos.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);

        granary::app_pc rep_stos_dep((granary::app_pc) unwatched_rep_stos_dep);
        granary::basic_block call_rep_stos_dep(granary::code_cache::find(
            rep_stos_dep, policy));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_stos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);

        granary::app_pc rep_wstos_dep((granary::app_pc) watched_rep_stos_dep);
        granary::basic_block call_rep_wstos_dep(granary::code_cache::find(
            rep_wstos_dep, policy));

        WP_STRING_FOO_ARRAY[0] = 0;
        WP_STRING_FOO_ARRAY[1] = 0;
        call_rep_wstos_dep.call<void>();
        ASSERT(1 == WP_STRING_FOO_ARRAY[0] && 1 == WP_STRING_FOO_ARRAY[1]);


        granary::app_pc rep_movs((granary::app_pc) unwatched_rep_movs);
        granary::basic_block call_rep_movs(granary::code_cache::find(
            rep_movs, policy));

        WP_STRING_FOO_ARRAY[0] = ~0ULL;
        WP_STRING_FOO_ARRAY[1] = ~0ULL;
        WP_STRING_FOO_ARRAY_DEST[0] = 0ULL;
        WP_STRING_FOO_ARRAY_DEST[1] = 0ULL;
        call_rep_movs.call<void>();
        ASSERT(~0ULL == WP_STRING_FOO_ARRAY[0]
            && ~0ULL == WP_STRING_FOO_ARRAY[1]
            && ~0ULL == WP_STRING_FOO_ARRAY_DEST[1]
            && ~0ULL == WP_STRING_FOO_ARRAY_DEST[1]);

        granary::app_pc rep_movs_memmove((granary::app_pc) unwatched_rep_movs_memmove);
        granary::basic_block call_rep_movs_memmove(granary::code_cache::find(
            rep_movs_memmove, policy));

        WP_STRING_FOO_ARRAY[0] = ~0ULL;
        WP_STRING_FOO_ARRAY[1] = ~0ULL;
        WP_STRING_FOO_ARRAY_DEST[0] = 0ULL;
        WP_STRING_FOO_ARRAY_DEST[1] = 0ULL;
        call_rep_movs_memmove.call<void>();
        ASSERT(~0ULL == WP_STRING_FOO_ARRAY[0]
            && ~0ULL == WP_STRING_FOO_ARRAY[1]
            && ~0ULL == WP_STRING_FOO_ARRAY_DEST[1]
            && ~0ULL == WP_STRING_FOO_ARRAY_DEST[1]);
    }

    ADD_TEST(string_watched_correctly,
        "Test that string instructions are correctly watched.")
}

#endif /* CONFIG_RUN_TEST_CASES */
