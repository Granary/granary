/*
 * attach.asm
 *
 *   Copyright: Copyright 2013 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

DECLARE_FUNC(_ZN7granary6attachENS_22instrumentation_policyE)
GLOBAL_LABEL(_ZN7granary6attachENS_22instrumentation_policyE:)
    lea (%rsp), %ARG2;
#ifdef __clang__
    lea SHARED_SYMBOL(_ZN7granary9do_attachENS_22instrumentation_policyEPPh), %rax;
    jmpq *%rax;
#else
    jmpq SHARED_SYMBOL(_ZN7granary9do_attachENS_22instrumentation_policyEPPh);
#endif
END_FUNC(ZN7granary6attachEv)

END_FILE
