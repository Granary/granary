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


/// Left shift for partial indexes to get the partial index into the high
/// position.
#define PARTIAL_INDEX_LSH \
    (64 - (WP_PARTIAL_INDEX_WIDTH + WP_PARTIAL_INDEX_GRANULARITY))


/// Right shift for partial indexes to get it into the low position.
#define PARTIAL_INDEX_RSH (64 - WP_PARTIAL_INDEX_WIDTH)


/// Right shift for counter indexes to get it into the low position.
#define COUNTER_INDEX_RSH (64 - WP_COUNTER_INDEX_WIDTH)


/// Define a register-specific and operand-size specific bounds checker.
#define BOUNDS_CHECKER(reg, size) \
    DECLARE_FUNC(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg)) @N@\
    GLOBAL_LABEL(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg):) @N@@N@\
        \
        COMMENT(Spill some registers. The DEST and TABLE registers are) \
        COMMENT(guaranteed to not be RAX. We unconditionally spill RAX) \
        COMMENT(because we will clobber its low order bits with the) \
        COMMENT(arithmetic flags.) \
        pushq %rax;@N@\
        pushq %DESC(reg);@N@\
        pushq %TABLE(reg);@N@\
        @N@\
        \
        COMMENT(Save the register so we can restore it later when we need to) \
        COMMENT(compare its lower order bits against the bounds.) \
        pushq %reg;@N@\
        @N@\
        \
        COMMENT(Save the watched address into a non-RAX reg before we clobber) \
        COMMENT(RAX for saving the flags.) \
        mov %reg, %DESC(reg);@N@\
        IF_PARTIAL_INDEX( mov %reg, %TABLE(reg);@N@ )\
        @N@\
        \
        COMMENT(Save the flags.) \
        lahf;@N@\
        seto %al;@N@\
        @N@\
        \
        COMMENT(Compute the combined index.) \
        IF_PARTIAL_INDEX( shl $ PARTIAL_INDEX_LSH, %TABLE(reg);@N@ ) \
        IF_PARTIAL_INDEX( shr $ PARTIAL_INDEX_RSH, %TABLE(reg);@N@ ) \
        shr $ COUNTER_INDEX_RSH, %DESC(reg);@N@ \
        IF_PARTIAL_INDEX( shl $ WP_PARTIAL_INDEX_WIDTH, %DESC(reg);@N@ ) \
        IF_PARTIAL_INDEX( or %TABLE(reg), %DESC(reg);@N@ ) \
        @N@\
        \
        COMMENT(Get the descriptor pointer from the index.) \
        lea SYMBOL(_ZN6client2wp11DESCRIPTORSE)(%rip), %TABLE(reg);@N@\
        lea (%TABLE(reg),%DESC(reg),1), %DESC(reg);@N@\
        popq %TABLE(reg);@N@\
        movq (%DESC(reg)), %DESC(reg);@N@\
        @N@\
        \
        COMMENT(Compare against the lower bound.) \
        cmpw %REG64_TO_REG16(TABLE(reg)), (%DESC(reg));@N@\
        jl .CAT(Lbounds_check_fail_, CAT(reg, size));@N@\
        @N@\
        \
        COMMENT(Compare against the upper bound.) \
        addq $ size, %TABLE(reg); \
        cmpw 2(%DESC(reg)), %REG64_TO_REG16(TABLE(reg));@N@\
        jle .CAT(Lbounds_check_fail_, CAT(reg, size));@N@\
        jmp .CAT(Lbounds_check_passed_, CAT(reg, size));@N@\
        @N@\
        \
    .CAT(Lbounds_check_fail_, CAT(reg, size)): @N@\
        COMMENT(Trigger an overflow exception if a bounds check fails.) \
        int $4;@N@\
        @N@\
        \
    .CAT(Lbounds_check_passed_, CAT(reg, size)): @N@\
        COMMENT(Restore the flags.) \
        addb $0x7F, %al;@N@\
        sahf;@N@\
        @N@\
        \
        COMMENT(Unspill.) \
        popq %TABLE(reg);@N@\
        popq %DESC(reg);@N@\
        popq %rax;@N@\
        ret;@N@\
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
GLOBAL_LABEL(granary_first_bounds_checker:)
ALL_REGS(DEFINE_CHECKERS, DEFINE_CHECKER)
GLOBAL_LABEL(granary_last_bounds_checker:)

END_FILE
