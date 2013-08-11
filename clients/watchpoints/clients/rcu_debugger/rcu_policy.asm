/*
 * rcu_policy.asm
 *
 *  Created on: 2013-08-09
 *      Author: akshayk
 */


#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

#include "clients/watchpoints/config.h"

START_FILE

    .extern SYMBOL(_ZN6client2wp11DESCRIPTORSE)
    .extern SYMBOL(_ZN6client2wp18verify_read_policyEmPh)

    /// Watchpoint descriptor.
#define DESC(reg) SPILL_REG_NOT_RAX(reg)


/// Table address, as well as temporary for calculating the descriptor index.
#define TABLE(reg) SPILL_REG_NOT_RAX(DESC(reg))


/// Define a register-specific and operand-size specific bounds checker.
#define DESCRIPTOR_READ_ACCESSOR(reg) \
    DECLARE_FUNC(CAT(granary_rcu_policy_read_, reg)) @N@\
    GLOBAL_LABEL(CAT(granary_rcu_policy_read_, reg):) @N@@N@\
        COMMENT(Save the flags.) \
        pushf;@N@\
        \
        COMMENT(Save all register state.)\
        @PUSH_ALL_REGS@ @N@\
        @N@\
        \
        COMMENT(Save the overflowed address and the return address as)\
        COMMENT(arguments for a common overflow handler.)\
        movq %reg, %ARG1; @N@\
        mov 128(%rsp), %ARG2; @N@\
        @N@\
        \
        COMMENT(Align the stack.)\
        push %rsp; @N@\
        push (%rsp); @N@\
        and $-0x10, %rsp; @N@\
        @N@\
        \
        COMMENT(Call out to our common handler.)\
        callq SYMBOL(_ZN6client2wp18verify_read_policyEmPh); @N@\
        COMMENT(Unalign the stack.)\
        mov 8(%rsp), %rsp; @N@\
        @N@\
        \
        @N@\
        \
        COMMENT(Restore all register state.)\
        @POP_ALL_REGS@ @N@\
        @N@\
        \
        COMMENT(Restore the flags.) \
        popf;@N@\
        @N@\
        ret;@N@\
    END_FUNC(CAT(granary_rcu_policy_read_, reg)) @N@@N@@N@

#define DESCRIPTOR_READ_ACCESSORS(reg, rest) \
    DESCRIPTOR_READ_ACCESSOR(reg) \
    rest

ALL_REGS(DESCRIPTOR_READ_ACCESSORS, DESCRIPTOR_READ_ACCESSOR)


END_FILE

