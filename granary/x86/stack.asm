
#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

.extern SYMBOL(granary_get_private_stack_top)

DECLARE_FUNC(granary_enter_private_stack)
GLOBAL_LABEL(granary_enter_private_stack:)
    // The compiler that will call this function has respected the ABI, so we
    // only need to protect registers we explicitly use
    push %ARG1
    push %ARG2

    // Call the helper that will give us our current private stack address
    // NB: After call, %REG_XAX is that address
    call EXTERN_SYMBOL(granary_get_private_stack_top)

    pop %ARG2
    pop %ARG1

    // Increment the depth counter, and finish up if this is not the first time
    // entering (i.e. we're already on the private stack)
    incl (%rax)
    cmp $1, (%rax)
    jne .enter_end

    // Switch to private stack
    xchg %rax, %rsp

    // Save the RSP
    push %rax

    // Set the RIP we'll return back to
    push (%rax)

.enter_end:

    ret

    END_FUNC(granary_enter_private_stack)

DECLARE_FUNC(granary_exit_private_stack)
GLOBAL_LABEL(granary_exit_private_stack:)
    // The compiler that will call this function has respected the ABI, so we
    // only need to protect registers we explicitly use
    push %ARG1
    push %ARG2

    // Call the helper that will give us our current private stack address
    // NB: After call, %REG_XAX is that address
    call EXTERN_SYMBOL(granary_get_private_stack_top)

    pop %ARG2
    pop %ARG1

    decl (%rax)
    cmp $0, (%rax)
    jne .exit_end

    // Grab the return RIP from our current stack and stash it at the 2nd to
    // top entry of the private stack
    pop -16(%rax)

    // Restore the saved RSP from the 1st entry at the top of the private stack
    mov -8(%rax), %rsp

    // Set the RIP we'll return to
    mov -16(%rax), %rax
    mov %rax, (%rsp)

.exit_end:

    ret

    END_FUNC(granary_exit_private_stack)

END_FILE

