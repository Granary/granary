/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * 1src_1dst.cc
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#include "granary/test.h"
#include "clients/watchpoints/policies/null_policy.h"


#define PUSH_LAST_REG(reg) \
    "push %" #reg ";"

#define PUSH_REG(reg, rest) \
    PUSH_LAST_REG(reg) \
    rest

#define POP_LAST_REG(reg) \
    "pop %" #reg ";"

#define POP_REG(reg, rest) \
    rest \
    POP_LAST_REG(reg)

#define PUSHA ALL_REGS(PUSH_REG, PUSH_LAST_REG)
#define POPA ALL_REGS(POP_REG, POP_LAST_REG)

namespace test {

#if GRANARY_IN_KERNEL
#   define MASK_OP "andq"
#else
#   define MASK_OP "orq"
#endif

    extern "C" {
        uint64_t WP_MOV_FOO = 0;
        uint64_t WP_MOV_MASK = client::wp::DISTINGUISHING_BIT_MASK;
    }

    static void unwatched_mov_to_mem(void) throw() {
        ASM(
            "movq $WP_MOV_FOO, %rax;"
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);"
        );
    }

    static void watched_mov_to_mem(void) throw() {
        ASM(
            "movq WP_MOV_MASK, %rax;"
            MASK_OP " $WP_MOV_FOO, %rax;" // mask the address of FOO
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);"
        );
    }

    static void unwatched_mov_to_mem_dep(void) throw() {
        ASM(
            "movq $WP_MOV_FOO, %rax;"
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);"

            PUSHA POPA // ensure all regs are live
        );
    }

    static void watched_mov_to_mem_dep(void) throw() {
        ASM(
            "movq WP_MOV_MASK, %rax;"
            MASK_OP " $WP_MOV_FOO, %rax;" // mask the address of FOO
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);"

            PUSHA POPA // ensure all regs are live
        );
    }


    /// Test that MOV instructions are correctly watched.
    static void mov_watched_correctly(void) {
        (void) WP_MOV_MASK;

        granary::app_pc mov((granary::app_pc) unwatched_mov_to_mem);
        granary::basic_block call_mov(granary::code_cache::find(
            mov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_mov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc wmov((granary::app_pc) watched_mov_to_mem);
        granary::basic_block call_wmov(granary::code_cache::find(
            wmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_wmov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc dmov((granary::app_pc) unwatched_mov_to_mem_dep);
        granary::basic_block call_dmov(granary::code_cache::find(
            dmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_dmov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc wdmov((granary::app_pc) watched_mov_to_mem_dep);
        granary::basic_block call_wdmov(granary::code_cache::find(
            wdmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_wdmov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);
    }


    ADD_TEST(mov_watched_correctly,
        "Test that MOV instructions are correctly watched.")
}


