/*
 * cpu.asm
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/x86/asm_defines.asm"

START_FILE

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
    lock;
    movq %ARG1, (%ARG2);
    ret;
END_FUNC(granary_atomic_write8)


END_FILE

