/* Copyright 2012-2013 Peter Goodman, all rights reserved. */

#include "granary/x86/asm_defines.asm"

START_FILE


/// Returns %gs:0.
DECLARE_FUNC(granary_get_gs_base)
GLOBAL_LABEL(granary_get_gs_base:)
    movq %gs:0, %rax;
    ret;
END_FUNC(granary_get_gs_base)


/// Declare a Granary-version of memcpy. This is not equivalent to the
/// generalized memcpy: it does not care about alignment or overlaps.
DECLARE_FUNC(granary_memcpy)
GLOBAL_LABEL(granary_memcpy:)
    movq %ARG3, %rcx; // number of bytes to set.
    movq %ARG1, %rdx; // store for return.      Note: ARG3 == rdx.
    rep movsb;
    movq %rdx, %rax;
    ret;
END_FUNC(granary_memcpy)


/// Declare a Granary-version of memset. This is not equivalent to the
/// generalized memcpy: it does not care about alignment or overlaps.
DECLARE_FUNC(granary_memset)
GLOBAL_LABEL(granary_memset:)
    movq %ARG2, %rax; // value to set.
    movq %ARG3, %rcx; // number of bytes to set.
    movq %ARG1, %rdx; // store for return.      Note: ARG3 == rdx.
    rep stosb; // mov %AL into (RDI), RCX times
    movq %rdx, %rax;
    ret;
END_FUNC(granary_memset)


/// Declare a Granary-version of memcmp. This is not equivalent to the
/// generalized memcpy: it does not care about alignment or overlaps.
DECLARE_FUNC(granary_memcmp)
GLOBAL_LABEL(granary_memcmp:)
.granary_memcmp_next_byte:
    test %ARG3, %ARG3;
    movq $0, %rax;
    jz .granary_memcmp_done;

    movb (%ARG1), %al;
    subb (%ARG2), %al;
    jnz .granary_memcmp_done;

    // Go to the next byte.
    sub $1, %ARG3;
    add $1, %ARG1;
    add $1, %ARG2;
    jmp .granary_memcmp_next_byte; // tail-call to next byte.

    // No more bytes to compare, or we found a difference.
.granary_memcmp_done:
    ret;
END_FUNC(granary_memcmp)


/// Returns %fs:0.
DECLARE_FUNC(granary_get_fs_base)
GLOBAL_LABEL(granary_get_fs_base:)
    movq %fs:0, %rax;
    ret;
END_FUNC(granary_get_fs_base)


/// Hash function for an address going into the IBL. Takes in the base address
/// of the table in ARG1 and the address to hash in ARG2 and returns the address
/// of the entry.
DECLARE_FUNC(granary_ibl_hash)
GLOBAL_LABEL(granary_ibl_hash:)
    movq %ARG2, %rax;

    rcr $4, %al;
    xchg %al, %ah;
    shl $4, %ax;

    movzwl %ax, %eax;
    add %ARG1, %rax;
    ret
END_FUNC(granary_ibl_hash)


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

