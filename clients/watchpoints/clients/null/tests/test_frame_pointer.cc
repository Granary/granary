/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_frame_pointer.cc
 *
 *  Created on: 2013-05-16
 *      Author: Peter Goodman
 */


// mov    %eax,-0x8(%rbp)


#include "granary/test.h"

#if CONFIG_DEBUG_RUN_TEST_CASES

#include "clients/watchpoints/clients/null/instrument.h"
#include "clients/watchpoints/clients/null/tests/pp.h"

#if !WP_IGNORE_FRAME_POINTER

namespace test {

#if CONFIG_ENV_KERNEL
#   define MASK_OP "andq"
#else
#   define MASK_OP "orq"
#endif

    extern "C" {
        uint32_t WP_FP_FOO = 0;
        uint64_t WP_FP_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }


    static void unwatched_move_fp(void) {
        ASM(
            "pushq %rbp;"
            "xorq %rax, %rax;"
            "movq $" TO_STRING(SYMBOL(WP_FP_FOO)) ", %rbp;"
            "addq $0x8, %rbp;"
            "mov %eax, -0x8(%rbp);"
            "jmp 1f; 1: nop;" // ensure all regs are live
            "popq %rbp;"
        );
    }


    static void watched_move_fp(void) {
        ASM(
            "pushq %rbp;"
            "xorq %rax, %rax;"
            "movq " TO_STRING(SYMBOL(WP_FP_MASK)) ", %rbp;"
            MASK_OP " $" TO_STRING(SYMBOL(WP_FP_FOO)) ", %rbp;" // mask the address of FOO
            "addq $0x8, %rbp;"
            "mov %eax, -0x8(%rbp);"
            "jmp 1f; 1: nop;" // ensure all regs are live
            "popq %rbp;"
        );
    }


    /// Test that arithmetic instructions are correctly watched.
    static void frame_pointer_watched_correctly(void) {
        (void) WP_FP_FOO;
        (void) WP_FP_MASK;


        granary::app_pc mov((granary::app_pc) unwatched_move_fp);
        granary::basic_block call_mov(granary::code_cache::find(
            mov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_FP_FOO = 1;
        call_mov.call<void>();
        ASSERT(!WP_FP_FOO);


        granary::app_pc wmov((granary::app_pc) watched_move_fp);
        granary::basic_block call_wmov(granary::code_cache::find(
            wmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_FP_FOO = 1;
        call_wmov.call<void>();
        ASSERT(!WP_FP_FOO);
    }

    ADD_TEST(frame_pointer_watched_correctly,
        "Test that using watchpoint instrumentation on instructions with "
        "memory operands containing frame pointers works correctly.")
}
#endif /* WP_IGNORE_FRAME_POINTER */
#endif /* CONFIG_DEBUG_RUN_TEST_CASES */

