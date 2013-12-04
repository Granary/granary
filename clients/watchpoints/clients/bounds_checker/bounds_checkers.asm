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

#define DESCRIPTORS SYMBOL(_ZN6client2wp11DESCRIPTORSE)
#define VISIT_OVERFLOW SYMBOL(_ZN6client2wp14visit_overflowEmjPPh)

    .extern DESCRIPTORS
    .extern VISIT_OVERFLOW


DECLARE_FUNC(granary_detected_overflow)
GLOBAL_LABEL(granary_detected_overflow:)
    // Arg1 (rdi) has the watched address.
    // Arg2 (rsi) has the operand size.

    // Get the return address into the basic block as Arg3 (rdx).
    lea 32(%rsp), %rdx;

    // Save the scratch registers.
    push %rcx;
    push %r8;
    push %r9;
    push %r10;
    push %r11;

    call VISIT_OVERFLOW;

    // Restore scratch registers.
    pop %r11;
    pop %r10;
    pop %r9;
    pop %r8;
    pop %rcx;

    // Restore the flags.
    sahf;
    pop %rax;
    pop %rdx;
    pop %rsi;
    pop %rdi;
    ret;
END_FUNC(granary_detected_overflow)


#define BOUNDS_CHECKER(reg, size) \
    DECLARE_FUNC(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg)) @N@\
    GLOBAL_LABEL(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg):) @N@@N@\
    push %rdi; @N@\
    mov %reg, %rdi; @N@\
    @N@\
    COMMENT(Tail-call to a generic bounds checker.) @N@\
    jmp SHARED_SYMBOL(CAT(granary_bounds_check_, size)); @N@\
    END_FUNC(CAT(CAT(granary_bounds_check_, CAT(size, _)), reg)) @N@@N@@N@


#define GENERIC_BOUNDS_CHECKER(size) \
    DECLARE_FUNC(CAT(granary_bounds_check_, size)) @N@\
    GLOBAL_LABEL(CAT(granary_bounds_check_, size):) @N@@N@\
    push %rsi; @N@\
    push %rdx; @N@\
    push %rax; @N@\
    lahf; COMMENT(Save the arithmetic flags) @N@\
    @N@\
    COMMENT(Get the index into RDX.) @N@\
    mov %rdi, %rdx; COMMENT(Copy the watched address into RDX.)@N@\
    shr $49, %rdx; COMMENT(Shift the index into the low 15 bits.)@N@\
    shl $4, %rdx; COMMENT(Scale the index by sizeof(bound_descriptor).)@N@\
    @N@\
    COMMENT(Get a pointer to the descriptor. The descriptor table is an array) @N@\
    COMMENT(of bound_descriptor structures, each of which is 16 bytes.) @N@\
    lea DESCRIPTORS(%rip), %rsi; @N@\
    add %rdx, %rsi; COMMENT(Add the scaled index to &(DESCRIPTORS[0]).)@N@\
    @N@\
    COMMENT(Check the lower bounds against the low 32 bits of the watched address.) @N@\
    cmp (%rsi), %edi; @N@\
    jl .CAT(Lgranary_underflow_, size); @N@\
    @N@\
    COMMENT(Check the upper bounds against the low 32 bits of the watched address.) @N@\
    mov 4(%rsi), %rsi; @N@\
    sub $ size, %rsi; @N@\
    cmp %edi, %esi; @N@\
    jmp .CAT(Lgranary_done_, size); @N@\
.CAT(Lgranary_underflow_, size): @N@\
    mov $ size, %rsi; @N@\
    jmp SHARED_SYMBOL(granary_detected_overflow); @N@\
.CAT(Lgranary_overflow_, size): @N@\
    mov $ size, %rsi; @N@\
    jmp SHARED_SYMBOL(granary_detected_overflow); @N@\
.CAT(Lgranary_done_, size): @N@\
    sahf; COMMENT(Restore the arithmetic flags.) @N@\
    pop %rax; @N@\
    pop %rdx; @N@\
    pop %rsi; @N@\
    pop %rdi; @N@\
    ret; @N@\
    END_FUNC(CAT(granary_bounds_check_, size)) @N@@N@@N@


GENERIC_BOUNDS_CHECKER(1)
GENERIC_BOUNDS_CHECKER(2)
GENERIC_BOUNDS_CHECKER(4)
GENERIC_BOUNDS_CHECKER(8)
GENERIC_BOUNDS_CHECKER(16)


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
