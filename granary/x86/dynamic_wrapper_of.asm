
#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

.extern SYMBOL(granary_dynamic_wrapper_of_impl)

// Original function prototype:
//      app_pc dynamic_wrapper_of(app_pc wrapper, app_pc wrappee) throw();
//
DECLARE_FUNC(_ZN7granary18dynamic_wrapper_ofEPhS0_)
GLOBAL_LABEL(_ZN7granary18dynamic_wrapper_ofEPhS0_:)

    // If we're in the kernel then we need to disable interrupts, as the stack
    // switching stuff switches us to a CPU-private stack.
    IF_KERNEL(pushf)
    IF_KERNEL(cli)

    ENTER_PRIVATE_STACK()
    call EXTERN_SYMBOL(granary_dynamic_wrapper_of_impl)
    mov %rax, %ARG1 // Save the return value past the stack switch.
    EXIT_PRIVATE_STACK()
    mov %ARG1, %rax

    IF_KERNEL(popf)
    ret

    END_FUNC(_ZN7granary18dynamic_wrapper_ofEPhS0_)

END_FILE
