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

#include "clients/watchpoints/config.h"

    .extern SYMBOL(_ZN6client2wp11DESCRIPTORSE)

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
#define BOUNDS_CHECKER(reg, size) \
    DECLARE_FUNC(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg)) @N@\
    GLOBAL_LABEL(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg):) @N@\
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
        lea SYMBOL(_ZN6client2wp11DESCRIPTORSE)(%rip), %TABLE(reg); @N@\
        lea (%TABLE(reg),%DESC(reg),1), %DESC(reg); @N@\
        popq %TABLE(reg); @N@\
        movq (%DESC(reg)), %DESC(reg); @N@\
        @N@\
        COMMENT(Compare against the bounds.) \
        cmpw %REG64_TO_REG16(TABLE(reg)), (%DESC(reg)); @N@\
        jl .CAT(Lbounds_check_fail_, CAT(reg, size)); @N@\
        cmpw 2(%DESC(reg)), %REG64_TO_REG16(TABLE(reg)); @N@\
        jle .CAT(Lbounds_check_fail_, CAT(reg, size)); @N@\
        jmp .CAT(Lbounds_check_passed_, CAT(reg, size)); @N@\
        @N@\
    .CAT(Lbounds_check_fail_, CAT(reg, size)): @N@\
        COMMENT(Trigger an overflow exception if a bounds check fails.) \
        int $4; @N@\
        @N@\
    .CAT(Lbounds_check_passed_, CAT(reg, size)): @N@\
        COMMENT(Restore the flags.) \
        addb $0x7F, %al; @N@\
        sahf; @N@\
        @N@\
        COMMENT(Unspill.) \
        popq %TABLE(reg); @N@\
        popq %DESC(reg); @N@\
        popq %rax; @N@\
        ret; @N@\
    END_FUNC(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg)) @N@@N@@N@


/// Define a bounds checker and splat the rest of the checkers.
#define DEFINE_CHECKERS(reg, rest) \
    DEFINE_CHECKER(reg) \
    rest


/// Define the last bounds checker.
#define DEFINE_CHECKER(reg) \
    BOUNDS_CHECKER(reg, 1) \
    BOUNDS_CHECKER(reg, 2) \
    BOUNDS_CHECKER(reg, 4) \
    BOUNDS_CHECKER(reg, 8) \
    BOUNDS_CHECKER(reg, 16)


/// Define all of the bounds checkers.
ALL_REGS(DEFINE_CHECKERS, DEFINE_CHECKER)

END_FILE
