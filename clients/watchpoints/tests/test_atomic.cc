/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_atomic.cc
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

    static void unwatched_cmpxchg(void) throw() {
        ASM(
            "movq $WP_ATOMIC_FOO_32, %rbx;"
            "movq $1, %rcx;"
            "movq $0, %rax;"
            "cmpxchg %rcx, (%rbx);"
            PUSHA POPA // ensure all regs are live
        );
    }

    static void watched_cmpxchg(void) throw() {
        ASM(
            "movq WP_ATOMIC_MASK, %rbx;"
            MASK_OP " $WP_ATOMIC_FOO_32, %rbx;" // mask the address of FOO
            "movq $1, %rcx;"
            "movq $0, %rax;"
            "cmpxchg %rcx, (%rbx);"
            PUSHA POPA // ensure all regs are live
        );
    }

    /*
     * CMPXCHG8B m64
     *
     * Compare EDX:EAX with m64. If equal, set ZF
     * and load ECX:EBX into m64. Else, clear ZF and
     * load m64 into EDX:EAX.
     */

    static void unwatched_cmpxchg8b(void) throw() {
        ASM(
            "movq $WP_ATOMIC_FOO, %r8;"

            "movq $0, %rcx;" // ECX:EBX = 1
            "movq $1, %rbx;"

            "movq $0, %rax;" // EAX:EDX = 0
            "movq $0, %rdx;"

            "lock; cmpxchg8b (%r8);"
            PUSHA POPA // ensure all regs are live
        );
    }

    static void watched_cmpxchg8b(void) throw() {
        ASM(
            "movq WP_ATOMIC_MASK, %r8;"
            MASK_OP " $WP_ATOMIC_FOO, %r8;" // mask the address of FOO

            "movq $0, %rcx;" // ECX:EBX = 1
            "movq $1, %rbx;"

            "movq $0, %rax;" // EAX:EDX = 0
            "movq $0, %rdx;"

            "lock; cmpxchg8b (%r8);"
            PUSHA POPA // ensure all regs are live
        );
    }

    /// Test that arithmetic instructions are correctly watched.
    static void atomic_watched_correctly(void) {
        (void) WP_ATOMIC_FOO;
        (void) WP_ATOMIC_FOO_32;
        (void) WP_ATOMIC_MASK;

        granary::app_pc cmpxchg((granary::app_pc) unwatched_cmpxchg);
        granary::basic_block call_cmpxchg(granary::code_cache::find(
            cmpxchg, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ATOMIC_FOO_32 = 0;
        call_cmpxchg.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO_32);

        granary::app_pc wcmpxchg((granary::app_pc) watched_cmpxchg);
        granary::basic_block call_wcmpxchg(granary::code_cache::find(
            wcmpxchg, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ATOMIC_FOO_32 = 0;
        call_wcmpxchg.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO_32);


        granary::app_pc cmpxchg8b((granary::app_pc) unwatched_cmpxchg8b);
        granary::basic_block call_cmpxchg8b(granary::code_cache::find(
            cmpxchg8b, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ATOMIC_FOO = 0;
        call_cmpxchg8b.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO);

        granary::app_pc wcmpxchg8b((granary::app_pc) watched_cmpxchg8b);
        granary::basic_block call_wcmpxchg8b(granary::code_cache::find(
            wcmpxchg8b, granary::policy_for<client::watchpoint_null_policy>()));

        WP_ATOMIC_FOO = 0;
        call_wcmpxchg8b.call<void>();
        ASSERT(1 == WP_ATOMIC_FOO);
    }

    ADD_TEST(atomic_watched_correctly,
        "Test that atomic instructions are correctly watched.")
}


