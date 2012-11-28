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

/// Handle direct branch lookup and patching
DECLARE_FUNC(granary_asm_direct_branch)
GLOBAL_LABEL(granary_asm_direct_branch:)


/// Make a direct branch patcher. This saves the machine state, then passes it
/// off to the patch_mangled_direct_cti function template in granary/mangle.h.
#define MAKE_DIRECT_BRANCH_PATCHER(op, op_len) \
    DECLARE_FUNC(granary_asm_direct_branch_ ## op) \
    GLOBAL_LABEL(granary_asm_direct_branch_ ## op:) \
    pushf; \
    IF_KERNEL(cli;) \
    PUSHA \
    mov %rsp, %ARG1; \
    call _ZN7granary24patch_mangled_direct_ctiIXadL_ZNS_ ## op_len ## op ## _EN9dynamorio7_opnd_tEEEEEvPNS_21direct_patch_mcontextE; \
    POPA \
    popf; \
    END_FUNC(granary_asm_direct_branch_ ## op)

FOR_EACH_DIRECT_BRANCH(MAKE_DIRECT_BRANCH_PATCHER)

END_FILE


