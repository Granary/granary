/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.asm
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

    .extern _ZN6client2wp11DESCRIPTORSE

START_FILE

/// Watchpoint descriptor.
#define DESC(reg) SPILL_REG_NOT_RAX(reg)


/// Table address, as well as temporary for calculating the descriptor index.
#define TABLE(reg) SPILL_REG_NOT_RAX(DESC(reg))


/// Useful for documentation
#define COMMENT(...)


/// Define a register-specific bounds checker.
///
/// Note: this assumes 16-bit descriptor indexes!
#define BOUNDS_CHECKER(reg) \
    DECLARE_FUNC(CAT(granary_bounds_check_, reg)) @N@\
    GLOBAL_LABEL(CAT(granary_bounds_check_, reg):) @N@\
        COMMENT(Spill some registers.) \
        pushq %rax; @N@\
        pushq %DESC(reg); @N@\
        pushq %TABLE(reg); @N@\
        pushq %reg; @N@\
        mov %reg, %DESC(reg); @N@\
        @N@\
        COMMENT(Save the flags.) \
        lahf; @N@\
        seto %al; @N@\
        @N@\
        COMMENT(Get the descriptor pointer from the index bits and the) \
        COMMENT(table address.) \
        shr $48, %DESC(reg); @N@\
        lea _ZN6client2wp11DESCRIPTORSE(%rip), %TABLE(reg); @N@\
        lea (%TABLE(reg),%DESC(reg),1), %DESC(reg); @N@\
        popq %TABLE(reg); @N@\
        movq (%DESC(reg)), %DESC(reg); @N@\
        @N@\
        COMMENT(Compare against the bounds.) \
        cmpw %REG64_TO_REG16(TABLE(reg)), (%DESC(reg)); @N@\
        jl .CAT(Lbounds_check_fail_, reg); @N@\
        cmpw 2(%DESC(reg)), %REG64_TO_REG16(TABLE(reg)); @N@\
        jle .CAT(Lbounds_check_fail_, reg); @N@\
        jmp .CAT(Lbounds_check_passed_, reg); @N@\
        @N@\
    .CAT(Lbounds_check_fail_, reg): @N@\
        COMMENT(Trigger an overflow exception if a bounds check fails.) \
        int $4; @N@\
        @N@\
    .CAT(Lbounds_check_passed_, reg): @N@\
        COMMENT(Restore the flags.) \
        addb $0x7F, %al; @N@\
        sahf; @N@\
        @N@\
        COMMENT(Unspill.) \
        popq %TABLE(reg); @N@\
        popq %DESC(reg); @N@\
        popq %rax; @N@\
        ret; @N@\
    END_FUNC(CAT(granary_bounds_check_, reg)) @N@@N@@N@


#if 0
#define BOUNDS_CHECKER(reg) \
    DECLARE_FUNC(CAT(granary_bounds_check_, reg)) \
    GLOBAL_LABEL(CAT(granary_bounds_check_, reg):) \
        COMMENT(Spill some registers.) \
        pushq %DESC(reg); \
        pushq %TABLE(reg); \
        \
        COMMENT(Get the swapped index bits into the low word of the table reg.)\
        movq %reg, %TABLE(reg); \
        bswapq %TABLE(reg); \
        \
        COMMENT(Swap the low order bytes to put the index bits into the) \
        COMMENT(right order. Unfortunately, BSWAP r16 is undefined.) \
        push %rax; \
        movw %REG64_TO_REG16(TABLE(reg)), %ax; \
        xchg %al, %ah; \
        movw %ax, %REG64_TO_REG16(TABLE(reg)); \
        pop %rax; \
        \
        movzwq %REG64_TO_REG16(TABLE(reg)), %DESC(reg); \
        lea _ZN6client2wp11DESCRIPTORSE(%rip), %TABLE(reg); \
        lea (%TABLE(reg),%DESC(reg),1), %DESC(reg); \
        \
        popq %TABLE(reg); \
        popq %DESC(reg); \
        ret; \
    END_FUNC(CAT(granary_bounds_check_, reg))
#endif

/// Define a bounds checker and splat the rest of the checkers.
#define DEFINE_NEXT_CHECKER(reg, rest) \
    BOUNDS_CHECKER(reg) \
    rest


/// Define the last bounds checker.
#define DEFINE_LAST_CHECKER(reg) \
    BOUNDS_CHECKER(reg)


/// Define all of the bounds checkers.
ALL_REGS(DEFINE_NEXT_CHECKER, DEFINE_LAST_CHECKER)

END_FILE
