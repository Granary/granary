/* Copyright 2012-2013 Peter Goodman, all rights reserved. */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

    .extern SYMBOL(granary_enter_private_stack)
    .extern SYMBOL(granary_exit_private_stack)

/// Defines a "template" for direct branches. This template is decoded and re-
/// encoded so as to generate many different direct branch stubs (one for each
/// direct jump instruction and policy).
DECLARE_FUNC(granary_asm_xmm_safe_direct_branch_template)
GLOBAL_LABEL(granary_asm_xmm_safe_direct_branch_template:)
    pushf

    // this is a Granary entry point, so disable interrupts if we are in the
    // kernel.
    IF_KERNEL(cli)

    PUSHA

    mov %rsp, %ARG1

    // Ensure 16-byte alignment of the stack; this assumes the stack pointer is
    // either 8 or 16-byte aligned.
    push %rsp
    push (%rsp)
    and $-0x10, %rsp

    PUSHA_XMM

    ENTER_PRIVATE_STACK()
    callq *%rax // Replaced by `make_direct_cti_patch_func`.
    EXIT_PRIVATE_STACK()

    POPA_XMM

    // Restore the old stack alignment.
    mov 8(%rsp), %rsp

    POPA
    popf

    // pop off the target address.
    lea 8(%rsp), %rsp;

    // return to the patch stub, which will jmp to the now patched function
    ret
    END_FUNC(granary_asm_xmm_safe_direct_branch_template)


/// Defines a "template" for direct branches. This template is decoded and re-
/// encoded so as to generate many different direct branch stubs (one for each
/// direct jump instruction and policy).
DECLARE_FUNC(granary_asm_direct_branch_template)
GLOBAL_LABEL(granary_asm_direct_branch_template:)
    pushf

    // this is a Granary entry point, so disable interrupts if we are in the
    // kernel.
    IF_KERNEL(cli)

    PUSHA

    mov %rsp, %ARG1

#if !GRANARY_IN_KERNEL
    // Ensure 16-byte alignment of the stack; this assumes the stack pointer is
    // either 8 or 16-byte aligned.
    push %rsp
    push (%rsp)
    and $-0x10, %rsp

    lea -32(%rsp), %rsp;
    movaps %xmm0, 16(%rsp);
    movaps %xmm1, (%rsp);
#endif

    ENTER_PRIVATE_STACK()
    callq *%rax // Replaced by `make_direct_cti_patch_func`.
    EXIT_PRIVATE_STACK()

#if !GRANARY_IN_KERNEL
    movaps (%rsp), %xmm1;
    movaps 16(%rsp), %xmm0;
    lea 32(%rsp), %rsp;

    // Restore the old stack alignment.
    mov 8(%rsp), %rsp
#endif

    POPA
    popf

    // Pop off the target address.
    lea 8(%rsp), %rsp;

    // return to the patch stub, which will jmp to the now patched function
    ret
    END_FUNC(granary_asm_direct_branch_template)

END_FILE
