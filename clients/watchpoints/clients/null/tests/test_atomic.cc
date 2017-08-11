/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_atomic.cc
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
        uint64_t WP_ATOMIC_FOO = 0;
        uint32_t WP_ATOMIC_FOO_32 = 0;
        uint64_t WP_ATOMIC_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }

    /*
     * CMPXCHG r/m64, r64
     *
     * Compare RAX with r/m64. If equal, ZF is set
     * and r64 is loaded into r/m64. Else, clear ZF
     * and load r/m64 into RAX.
     */

    static void unwatched_cmpxchg(void) {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_ATOMIC_FOO_32)) ", %rbx;"
            "movq $1, %rcx;"
            "movq $0, %rax;"
            "cmpxchg %rcx, (%rbx);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    static void watched_cmpxchg(void) {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_ATOMIC_MASK)) ", %rbx;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_ATOMIC_FOO_32)) ", %rbx;" // mask the address of FOO
            "movq $1, %rcx;"
            "movq $0, %rax;"
            "cmpxchg %rcx, (%rbx);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    /*
     * CMPXCHG8B m64
     *
     * Compare EDX:EAX with m64. If equal, set ZF
     * and load ECX:EBX into m64. Else, clear ZF and
     * load m64 into EDX:EAX.
     */

    static void unwatched_cmpxchg8b(void) {
        ASM(
            "movq $" TO_STRING(SYMBOL(WP_ATOMIC_FOO)) ", %r8;"

            "movq $0, %rcx;" // ECX:EBX = 1
            "movq $1, %rbx;"

            "movq $0, %rax;" // EAX:EDX = 0
            "movq $0, %rdx;"

            "lock; cmpxchg8b (%r8);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    static void watched_cmpxchg8b(void) {
        ASM(
            "movq " TO_STRING(SYMBOL(WP_ATOMIC_MASK)) ", %r8;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_ATOMIC_FOO)) ", %r8;" // mask the address of FOO

            "movq $0, %rcx;" // ECX:EBX = 1
            "movq $1, %rbx;"

            "movq $0, %rax;" // EAX:EDX = 0
            "movq $0, %rdx;"

            "lock; cmpxchg8b (%r8);"
            "jmp 1f; 1: nop;" // ensure all regs are live
        );
    }

    /// Test that arithmetic instructions are correctly watched.
    static void atomic_watched_correctly(void) {
        (void) WP_ATOMIC_FOO;
        (void) WP_ATOMIC_FOO_32;
        (void) WP_ATOMIC_MASK;

        granary::instrumentation_policy policy =
            granary::policy_for<client::watchpoint_null_policy>();
        policy.force_attach(true);

        granary::app_pc cmpxchg((granary::app_pc) unwatched_cmpxchg);
        granary::basic_block call_cmpxchg(granary::code_cache::find(
            cmpxchg, policy));

        WP_ATOMIC_FOO_32 = 0;
        call_cmpxchg.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO_32);

        granary::app_pc wcmpxchg((granary::app_pc) watched_cmpxchg);
        granary::basic_block call_wcmpxchg(granary::code_cache::find(
            wcmpxchg, policy));

        WP_ATOMIC_FOO_32 = 0;
        call_wcmpxchg.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO_32);


        granary::app_pc cmpxchg8b((granary::app_pc) unwatched_cmpxchg8b);
        granary::basic_block call_cmpxchg8b(granary::code_cache::find(
            cmpxchg8b, policy));

        WP_ATOMIC_FOO = 0;
        call_cmpxchg8b.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO);

        granary::app_pc wcmpxchg8b((granary::app_pc) watched_cmpxchg8b);
        granary::basic_block call_wcmpxchg8b(granary::code_cache::find(
            wcmpxchg8b, policy));

        WP_ATOMIC_FOO = 0;
        call_wcmpxchg8b.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO);
    }

    ADD_TEST(atomic_watched_correctly,
        "Test that atomic instructions are correctly watched.")
}

#endif /* CONFIG_DEBUG_RUN_TEST_CASES */
