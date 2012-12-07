/*
 * direct_branch.asm
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

/// Defines a "template" for direct branches. This template is decoded and re-
/// encoded so as to generate many different direct branch stubs (one for each
/// direct jump instruction and policy).
DECLARE_FUNC(granary_asm_direct_branch_template)
GLOBAL_LABEL(granary_asm_direct_branch_template:)
    pushf

    // padding for stack frame alignment
    lea -0x8(%rsp), %rsp

    // this is a Granary entry point, so disable interrupts if we are in the
    // kernel.
    IF_KERNEL(cli)

    PUSHA
    mov %rsp, %ARG1
1:
    call 1b;

    POPA
    lea 0x8(%rsp), %rsp
    popf

    // the direct branch stubs push on a relative address offset on the stack;
    // need to get rid of it without changing the flags
    lea 0x8(%rsp), %rsp

    // return to the now patched instruction
    ret
    END_FUNC(granary_asm_direct_branch_template)


/// Defines a "template" for direct calls. This template is decoded and re-
/// encoded so as to generate a direct call entry stub for each policy. This
/// is different from the direct branch template in that not as much state
/// needs to be saved.
DECLARE_FUNC(granary_asm_direct_call_template)
GLOBAL_LABEL(granary_asm_direct_call_template:)
    // this is a Granary entry point, so disable interrupts if we are in the
    // kernel.
    IF_KERNEL(pushf; cli)
    IF_USER(lea -0x8(%rsp), %rsp) // in user space, make sure things are 16-byte aligned
    PUSH_ARGS
    mov %rsp, %ARG1

1:
    call 1b;
    POP_ARGS
    IF_USER(lea 0x8(%rsp), %rsp)
    IF_KERNEL(popf)

    // the direct branch stubs push on a relative address offset on the stack;
    // need to get rid of it without changing the flags
    lea 0x8(%rsp), %rsp

    // return to the now patched instruction
    ret
    END_FUNC(granary_asm_direct_call_template)

END_FILE


