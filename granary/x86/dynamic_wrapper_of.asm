
#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

.extern SYMBOL(granary_get_private_stack_top)
.extern SYMBOL(granary_dynamic_wrapper_of__impl)

DECLARE_FUNC(granary_dynamic_wrapper_of)
GLOBAL_LABEL(granary_dynamic_wrapper_of:)
    // The compiler that will call this function has respected the ABI, so we
    // only need to protect registers we explicitly use

    push %ARG1
    push %ARG2

    // Call the helper that will give us our current private stack address
    // NB: After call, %REG_XAX is that address
    call EXTERN_SYMBOL(granary_get_private_stack_top)

    pop %ARG2
    pop %ARG1

    // Switch to new private stack
    xchg %REG_XAX, %REG_XSP

    // Save old user stack address
    push %REG_XAX
    push %REG_XAX

    // Ready to call the real implementation
    call EXTERN_SYMBOL(granary_dynamic_wrapper_of__impl)

    // Switch back to old user stack
    mov (%REG_XSP), %REG_XSP

    ret

    END_FUNC(granary_dynamic_wrapper_of)

END_FILE
