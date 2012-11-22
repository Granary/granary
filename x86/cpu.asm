/*
 * cpu.asm
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "x86/asm_defines.asm"

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

END_FILE

