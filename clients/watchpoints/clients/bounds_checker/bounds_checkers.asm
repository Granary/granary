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

START_FILE

    .extern SYMBOL(_ZN6client2wp11DESCRIPTORSE)
    .extern SYMBOL(_ZN6client2wp14visit_overflowEmPhj)


/// Watchpoint descriptor.
#define DESC(reg) SPILL_REG_NOT_RAX(reg)


/// Table address, as well as temporary for calculating the descriptor index.
#define TABLE(reg) SPILL_REG_NOT_RAX(DESC(reg))


/// Define a register-specific and operand-size specific bounds checker.
#define BOUNDS_CHECKER(reg, size) \
    DECLARE_FUNC(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg)) @N@\
    GLOBAL_LABEL(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg):) @N@@N@\
        COMMENT(Save the flags.) \
        pushf;@N@\
        \
        COMMENT(Spill some registers. The DEST and TABLE registers are) \
        COMMENT(guaranteed to not be RAX. We unconditionally spill RAX.) \
        pushq %rax;@N@\
        pushq %DESC(reg);@N@ \
        pushq %TABLE(reg);@N@ \
        @N@\
        \
        COMMENT(Save the watched address for indexing and in RAX) \
        mov %reg, %DESC(reg);@N@\
        IF_INHERITED_INDEX( mov %reg, %TABLE(reg);@N@ )\
        mov %reg, %rax; @N@\
        @N@\
        \
        COMMENT(Compute the combined index.) \
        IF_INHERITED_INDEX( shl $ WP_PARTIAL_INDEX_LSH, %TABLE(reg);@N@ ) \
        IF_INHERITED_INDEX( shr $ WP_PARTIAL_INDEX_RSH, %TABLE(reg);@N@ ) \
        shr $ WP_COUNTER_INDEX_RSH, %DESC(reg);@N@ \
        IF_INHERITED_INDEX( shl $ WP_PARTIAL_INDEX_WIDTH, %DESC(reg);@N@ ) \
        IF_INHERITED_INDEX( or %TABLE(reg), %DESC(reg);@N@ ) \
        @N@\
        \
        COMMENT(Get the descriptor pointer from the index.) \
        lea SYMBOL(_ZN6client2wp11DESCRIPTORSE)(%rip), %TABLE(reg);@N@\
        lea (%TABLE(reg),%DESC(reg),8), %DESC(reg);@N@\
        \
        movq (%DESC(reg)), %DESC(reg);@N@\
        @N@\
        \
        COMMENT(Compare against the lower bound.) \
        cmp (%DESC(reg)), %eax;@N@\
        jl .CAT(Lbounds_check_fail_, CAT(reg, size));@N@\
        @N@\
        \
        COMMENT(Compare against the upper bound.) \
        mov %rax, %TABLE(reg); \
        addq $ (size - 1), %TABLE(reg); \
        cmp %REG64_TO_REG32(TABLE(reg)), 4(%DESC(reg));@N@\
        jle .CAT(Lbounds_check_fail_, CAT(reg, size));@N@\
        jmp .CAT(Lbounds_check_passed_, CAT(reg, size));@N@\
        @N@\
        \
    .CAT(Lbounds_check_fail_, CAT(reg, size)): @N@\
        COMMENT(Trigger an overflow exception if a bounds check fails.) \
        callq SHARED_SYMBOL(CAT(granary_overflow_handler_, CAT(CAT(size, _), rax))); \
        @N@\
        \
    .CAT(Lbounds_check_passed_, CAT(reg, size)): @N@\
        \
        COMMENT(Unspill.) \
        popq %TABLE(reg);@N@\
        popq %DESC(reg);@N@\
        popq %rax;@N@\
        \
        COMMENT(Restore the flags.) \
        popf;@N@\
        @N@\
        ret;@N@\
    END_FUNC(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg)) @N@@N@@N@


/// An overflow handler.
#define OVERFLOW_HANDLER(reg, size) \
    DECLARE_FUNC(CAT(granary_overflow_handler_, CAT(CAT(size, _), reg))) @N@\
    GLOBAL_LABEL(CAT(granary_overflow_handler_, CAT(CAT(size, _), reg)):) @N@\
        COMMENT(Save all register state.)\
        @PUSH_ALL_REGS@ @N@\
        @N@\
        \
        COMMENT(Save the overflowed address and the return address as)\
        COMMENT(arguments for a common overflow handler.)\
        movq %reg, %ARG1; @N@\
        mov 160(%rsp), %ARG2; @N@\
        movq $ size, %ARG3; @N@\
        @N@\
        \
        COMMENT(Align the stack.)\
        push %rsp; @N@\
        push (%rsp); @N@\
        and $-0x10, %rsp; @N@\
        @N@\
        \
        IF_USER(COMMENT(Save the XMM registers.)) \
        IF_USER(@PUSH_ALL_XMM_REGS@) \
        IF_USER(@N@) \
        \
        COMMENT(Call out to our common handler.)\
        callq SYMBOL(_ZN6client2wp14visit_overflowEmPhj); @N@\
        @N@\
        \
        IF_USER(COMMENT(Restore the XMM registers.)) \
        IF_USER(@POP_ALL_XMM_REGS@) \
        IF_USER(@N@) \
        \
        COMMENT(Unalign the stack.)\
        mov 8(%rsp), %rsp; @N@\
        @N@\
        \
        COMMENT(Restore all register state.)\
        @POP_ALL_REGS@ @N@\
        ret;@N@\
    END_FUNC(CAT(granary_overflow_, CAT(CAT(size, _), reg))) @N@\
    @N@


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


/// Define an overflow handler and splat the rest of the checkers.
#define DEFINE_HANDLERS(reg, rest) \
    DEFINE_HANDLER(reg) \
    rest


/// Define the last overflow handler.
#define DEFINE_HANDLER(reg) \
    OVERFLOW_HANDLER(reg, 1) \
    OVERFLOW_HANDLER(reg, 2) \
    OVERFLOW_HANDLER(reg, 4) \
    OVERFLOW_HANDLER(reg, 8) \
    OVERFLOW_HANDLER(reg, 16)


/// Define all of the bounds checkers.
GLOBAL_LABEL(granary_first_bounds_checker:)
ALL_REGS(DEFINE_CHECKERS, DEFINE_CHECKER)
GLOBAL_LABEL(granary_last_bounds_checker:)

DEFINE_HANDLER(rax)

END_FILE
