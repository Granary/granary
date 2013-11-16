/* Copyright 2012-2013 Peter Goodman, all rights reserved. */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

DECLARE_FUNC(_ZN7granary6attachENS_22instrumentation_policyE)
GLOBAL_LABEL(_ZN7granary6attachENS_22instrumentation_policyE:)
    lea (%rsp), %ARG2;
#if !CONFIG_ENV_KERNEL
#   if defined(__clang__) && defined(__APPLE__)
    lea SHARED_SYMBOL(_ZN7granary9do_attachENS_22instrumentation_policyEPPh), %rax;
    jmpq *%rax;
#   else
    jmp SHARED_SYMBOL(_ZN7granary9do_attachENS_22instrumentation_policyEPPh);
#   endif
#else
    jmp SYMBOL(_ZN7granary9do_attachENS_22instrumentation_policyEPPh)
#endif
END_FUNC(ZN7granary6attachEv)

END_FILE
