
#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

.extern SYMBOL(granary_dynamic_wrapper_of__impl)

// Original function prototype:
//      app_pc dynamic_wrapper_of(app_pc wrapper, app_pc wrappee) throw();
//
DECLARE_FUNC(_ZN7granary18dynamic_wrapper_ofEPhS0_)
GLOBAL_LABEL(_ZN7granary18dynamic_wrapper_ofEPhS0_:)
    ENTER_PRIVATE_STACK()
    call EXTERN_SYMBOL(granary_dynamic_wrapper_of__impl)

    // Save the return value from the wrapper past this stack exit
    mov %rax, %ARG1
    EXIT_PRIVATE_STACK()
    mov %ARG1, %rax

    ret

    END_FUNC(_ZN7granary18dynamic_wrapper_ofEPhS0_)

END_FILE
