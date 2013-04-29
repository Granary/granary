/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * 1src_1dst.cc
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

#include "clients/watchpoints/policies/null_policy.h"
#include "clients/watchpoints/tests/pp.h"

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

    /// Simple un/watched, no flags dependencies, no register dependencies.

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

    /// Simple un/watched, no flags dependencies, with register dependencies.

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

    /// Simple un/watched, with flags dependencies, no register dependencies.

    static void unwatched_mov_to_mem_cf(void) throw() {
        ASM(
            "clc;" // set CF=0
            "movq $WP_MOV_FOO, %rax;"
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);" // on restore, CF=0
            "cmovc %rax, %rbx;" // should be a NOP, iff CF was restored to 0
            "movq %rbx, (%rax);"
        );
    }

    static void watched_mov_to_mem_cf(void) throw() {
        ASM(
            "movq WP_MOV_MASK, %rax;"
            MASK_OP " $WP_MOV_FOO, %rax;" // mask the address of FOO
            "clc;" // set CF=0
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);" // on restore, CF=0
            "cmovc %rax, %rbx;" // should be a NOP, iff CF was restored to 0
            "movq %rbx, (%rax);"
        );
    }

    /// Simple un/watched, with flags dependencies, with register dependencies.

    static void unwatched_mov_to_mem_cf_dep(void) throw() {
        ASM(
            "clc;" // set CF=0
            "movq $WP_MOV_FOO, %rax;"
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);" // on restore, CF=0
            PUSHA POPA // ensure all regs are live
            "cmovc %rax, %rbx;" // should be a NOP, iff CF was restored to 0
            "movq %rbx, (%rax);"
        );
    }

    static void watched_mov_to_mem_cf_dep(void) throw() {
        ASM(
            "movq WP_MOV_MASK, %rax;"
            MASK_OP " $WP_MOV_FOO, %rax;" // mask the address of FOO
            "clc;" // set CF=0
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax);" // on restore, CF=0
            PUSHA POPA // ensure all regs are live
            "cmovc %rax, %rbx;" // should be a NOP, iff CF was restored to 0
            "movq %rbx, (%rax);"
        );
    }


    /// Test that a dead register is not restored at a bad time.
    static uint64_t unwatched_mov_from_mem_dead(void) throw() {
        register uint64_t ret asm("rax");
        ASM(
            "movq $WP_MOV_FOO, %%rax;"
            "movq (%%rax), %%rax;"
            "movq %%rax, %0;"
            : "=r"(ret)
        );
        return ret;
    }

    static uint64_t watched_mov_from_mem_dead(void) throw() {
        register uint64_t ret asm("rax");
        ASM(
            "movq WP_MOV_MASK, %%rax;"
            MASK_OP " $WP_MOV_FOO, %%rax;" // mask the address of FOO
            "clc;" // set CF=0
            "movq (%%rax), %%rax;"
            "movq %%rax, %0;"
            : "=r"(ret)
        );
        return ret;
    }


    /// Test that a dead register that can't scale to 8 bits is not restored at
    /// a bad time. This is only relevant for watchpoints that use 8 bits of
    /// taints.
    static uint64_t unwatched_mov_from_mem_dead_8(void) throw() {
        register uint64_t ret asm("rax");
        ASM(
            "movq $WP_MOV_FOO, %%rsi;"
            "movq (%%rsi), %%rsi;"
            "movq %%rsi, %0;"
            : "=r"(ret)
        );
        return ret;
    }


    /// Test MOV instructions that use the index register instead of the base
    /// register.

    static void unwatched_mov_to_mem_index(void) throw() {
        ASM(
            "movq $WP_MOV_FOO, %rax;"
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (,%rax,1);"
        );
    }

    static void watched_mov_to_mem_index(void) throw() {
        ASM(
            "movq WP_MOV_MASK, %rax;"
            MASK_OP " $WP_MOV_FOO, %rax;" // mask the address of FOO
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (,%rax,1);"
        );
    }

    static uint64_t unwatched_mov_from_mem_dead_index(void) throw() {
        register uint64_t ret asm("rax");
        ASM(
            "movq $WP_MOV_FOO, %%rax;"
            "movq (,%%rax,1), %%rax;"
            "movq %%rax, %0;"
            : "=r"(ret)
        );
        return ret;
    }

    static uint64_t watched_mov_from_mem_dead_index(void) throw() {
        register uint64_t ret asm("rax");
        ASM(
            "movq WP_MOV_MASK, %%rax;"
            MASK_OP " $WP_MOV_FOO, %%rax;" // mask the address of FOO
            "clc;" // set CF=0
            "movq (,%%rax,1), %%rax;"
            "movq %%rax, %0;"
            : "=r"(ret)
        );
        return ret;
    }

    /// Test MOV instructions that use both the base and index registers.
    static void unwatched_mov_to_mem_base_index(void) throw() {
        ASM(
            "movq $WP_MOV_FOO, %rax;"
            "movq $0, %rsi;"
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax,%rsi,1);"
        );
    }

    static void watched_mov_to_mem_base_index(void) throw() {
        ASM(
            "movq WP_MOV_MASK, %rax;"
            "movq $0, %rsi;"
            MASK_OP " $WP_MOV_FOO, %rax;" // mask the address of FOO
            "movq $0xDEADBEEF, %rbx;"
            "movq %rbx, (%rax,%rsi,1);"
        );
    }


    /// Test that MOV instructions are correctly watched.
    static void mov_watched_correctly(void) {
        (void) WP_MOV_MASK;

        // Simple un/watched, no flags dependencies, no register dependencies.

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

        // Simple un/watched, no flags dependencies, with register dependencies.

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

        // Simple un/watched, with flags dependencies, no register dependencies.

        granary::app_pc cmov((granary::app_pc) unwatched_mov_to_mem_cf);
        granary::basic_block call_cmov(granary::code_cache::find(
            cmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_cmov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc wcmov((granary::app_pc) watched_mov_to_mem_cf);
        granary::basic_block call_wcmov(granary::code_cache::find(
            wcmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_wcmov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        // Simple un/watched, with flags dependencies, with register
        // dependencies.

        granary::app_pc dcmov((granary::app_pc) unwatched_mov_to_mem_cf_dep);
        granary::basic_block call_dcmov(granary::code_cache::find(
            dcmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_dcmov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc wdcmov((granary::app_pc) watched_mov_to_mem_cf_dep);
        granary::basic_block call_wdcmov(granary::code_cache::find(
            wdcmov, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_wdcmov.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        // Test that a dead register is not restored at a bad time.
        granary::app_pc mov_dead((granary::app_pc) unwatched_mov_from_mem_dead);
        granary::basic_block call_mov_dead(granary::code_cache::find(
            mov_dead, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0xDEADBEEF;
        ASSERT(0xDEADBEEF == call_mov_dead.call<uint64_t>());


        // Test that a dead register is not restored at a bad time, and that it
        // works when the memory is watched.
        granary::app_pc wmov_dead((granary::app_pc) watched_mov_from_mem_dead);
        granary::basic_block call_wmov_dead(granary::code_cache::find(
            wmov_dead, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0xDEADBEEF;
        ASSERT(0xDEADBEEF == call_wmov_dead.call<uint64_t>());


        // Test that a dead register is not restored at a bad time.
        granary::app_pc mov_dead8((granary::app_pc) unwatched_mov_from_mem_dead_8);
        granary::basic_block call_mov_dead8(granary::code_cache::find(
            mov_dead8, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0xDEADBEEF;
        ASSERT(0xDEADBEEF == call_mov_dead8.call<uint64_t>());

        granary::app_pc mov_index((granary::app_pc) unwatched_mov_to_mem_index);
        granary::basic_block call_mov_index(granary::code_cache::find(
            mov_index, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_mov_index.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc wmov_index((granary::app_pc) watched_mov_to_mem_index);
        granary::basic_block call_wmov_index(granary::code_cache::find(
            wmov_index, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_wmov_index.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc mov_dead_index((granary::app_pc) unwatched_mov_from_mem_dead_index);
        granary::basic_block call_mov_dead_index(granary::code_cache::find(
            mov_dead_index, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0xDEADBEEF;
        ASSERT(0xDEADBEEF == call_mov_dead_index.call<uint64_t>());

        granary::app_pc wmov_dead_index((granary::app_pc) watched_mov_from_mem_dead_index);
        granary::basic_block call_wmov_dead_index(granary::code_cache::find(
            wmov_dead_index, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0xDEADBEEF;
        ASSERT(0xDEADBEEF == call_wmov_dead_index.call<uint64_t>());

        granary::app_pc mov_base_index((granary::app_pc) unwatched_mov_to_mem_base_index);
        granary::basic_block call_mov_base_index(granary::code_cache::find(
            mov_base_index, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_mov_base_index.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);

        granary::app_pc wmov_base_index((granary::app_pc) watched_mov_to_mem_base_index);
        granary::basic_block call_wmov_base_index(granary::code_cache::find(
            wmov_base_index, granary::policy_for<client::watchpoint_null_policy>()));

        WP_MOV_FOO = 0;
        call_wmov_base_index.call<void>();
        ASSERT(0xDEADBEEF == WP_MOV_FOO);
    }


    ADD_TEST(mov_watched_correctly,
        "Test that MOV instructions are correctly watched.")
}

#endif /* CONFIG_RUN_TEST_CASES */
