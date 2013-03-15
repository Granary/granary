/*
 * init.asm
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

#if GRANARY_IN_KERNEL

START_FILE

DECLARE_FUNC(granary_run_initialisers)
GLOBAL_LABEL(granary_run_initialisers:)
    .cfi_startproc
    push %rbp;
    mov %rsp, %rbp;

    // This is an automatically generated file, that is generated when Granary
    // is built.

    // the implication is that this particular file must be compiled *after*
    // all others.
#   include "granary/gen/kernel_init.S"

    pop %rbp;
    ret;
    .cfi_endproc
END_FUNC(granary_run_initialisers)

END_FILE

#endif
