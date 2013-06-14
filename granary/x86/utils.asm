/* Copyright 2012-2013 Peter Goodman, all rights reserved. */

#include "granary/x86/asm_defines.asm"

START_FILE


/// Returns %gs:0.
DECLARE_FUNC(granary_get_gs_base)
GLOBAL_LABEL(granary_get_gs_base:)
    movq %gs:0, %rax;
    ret;
END_FUNC(granary_get_gs_base)


/// Returns %fs:0.
DECLARE_FUNC(granary_get_fs_base)
GLOBAL_LABEL(granary_get_fs_base:)
    movq %fs:0, %rax;
    ret;
END_FUNC(granary_get_fs_base)


/// Get the current processor's APIC ID. This assumes that interrupts
/// are disabled.
DECLARE_FUNC(granary_asm_apic_id)
GLOBAL_LABEL(granary_asm_apic_id:)
    push %rbx;
    push %rcx;
    push %rdx;
    mov $1, %rax;
    cpuid;
    shr $23, %rbx;
    and $0xFF, %rbx;
    mov %rbx, %rax;
    ret;
END_FUNC(granary_asm_apic_id)


/// Atomically write 8 bytes to memory.
DECLARE_FUNC(granary_atomic_write8)
GLOBAL_LABEL(granary_atomic_write8:)
    lock xchg %ARG1, (%ARG2);
    ret;
END_FUNC(granary_atomic_write8)


DECLARE_FUNC(granary_get_stack_pointer)
GLOBAL_LABEL(granary_get_stack_pointer:)
    mov %rsp, %rax;
    lea 0x8(%rax), %rax;
    ret;
END_FUNC(granary_get_stack_pointer)


DECLARE_FUNC(granary_disable_interrupts)
GLOBAL_LABEL(granary_disable_interrupts:)
    pushf;
    cli;
    pop %rax;
    ret;
END_FUNC(granary_disable_interrupts)


DECLARE_FUNC(granary_load_flags)
GLOBAL_LABEL(granary_load_flags:)
    pushf;
    pop %rax;
    ret;
END_FUNC(granary_load_flags)


DECLARE_FUNC(granary_store_flags)
GLOBAL_LABEL(granary_store_flags:)
    push %ARG1;
    popf
    ret;
END_FUNC(granary_store_flags)


END_FILE

