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
    lea SYMBOL(_ZN7granary9do_attachENS_22instrumentation_policyEPPh)(%rip), %rax;
    jmp *%rax;
END_FUNC(ZN7granary6attachEv)

END_FILE
