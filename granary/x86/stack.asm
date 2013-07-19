
#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

START_FILE

.extern SYMBOL(granary_get_private_stack_top)

DECLARE_FUNC(granary_enter_private_stack)
GLOBAL_LABEL(granary_enter_private_stack:)
    // The compiler that will call this function has respected the ABI, so we
    // only need to protect registers we explicitly use
    pushq %ARG1
    pushq %ARG2

    // Call the helper that will give us our current private stack address
    // NB: After call, RAX is that address
    call EXTERN_SYMBOL(granary_get_private_stack_top)

    popq %ARG2
    popq %ARG1

    // As of here:
    // %rax - top of private stack (also address of nesting depth counter)

    // Compare the top 48-bits of the private stack address %rax and
    // whichever stack we are currently on %rsp
    movq %rax, %rbx
    xorq %rsp, %rbx
    shrq $16, %rbx
    test %rbx, %rbx

    // High 48-bits were the same, already on private stack!
    jz .enter_do_not_switch

.enter_switch_stack:
    // Get return address of this function call
    popq %rbx

    // Switch stack
    xchg %rsp, %rax

    // As of here:
    // %rsp - top of private stack (also address of nesting depth counter)
    // %rax - somewhere on the native stack

    // Save current native stack position
    // NB: two pushes to ensure 16 byte alignment
    pushq %rax
    pushq %rax

    // Tail-cail return
    jmpq *%rbx

.enter_do_not_switch:
    // Get return address of this function call
    popq %rbx

    // As of here (since we are already on the private stack per the test above):
    // %rsp - somewhere on the private stack
    // %rax - top of private stack (also address of nesting depth counter)

    // Save current private stack position
    // NB: two pushes to ensure 16 byte alignment (and make sure they are both
    // the same %rsp value!)
    movq %rsp, %rax
    pushq %rax
    pushq %rax

    // Tail-cail return
    jmpq *%rbx

    END_FUNC(granary_enter_private_stack)

DECLARE_FUNC(granary_exit_private_stack)
GLOBAL_LABEL(granary_exit_private_stack:)
    // Get return address
    pop %rbx

    // Unconditionally switch stacks
    pop %rsp

    // Tail-call return
    jmpq *%rbx

    END_FUNC(granary_exit_private_stack)

END_FILE

