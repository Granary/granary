/* Copyright 2012-2013 Peter Goodman, all rights reserved. */

#ifndef Granary_ASM_HELPERS_ASM_
#define Granary_ASM_HELPERS_ASM_

#define PUSH_LAST_REG(reg) \
    push %reg ;

#define PUSH_REG(reg, rest) \
    PUSH_LAST_REG(reg) \
    rest

#define POP_LAST_REG(reg) \
    pop %reg ;

#define POP_REG(reg, rest) \
    rest \
    POP_LAST_REG(reg)



#define PUSH_LAST_XMM_REG(off, reg) \
    movaps %reg, off(%rsp);

#define PUSH_XMM_REG(off, reg, rest) \
    PUSH_LAST_XMM_REG(off, reg) \
    rest

#define POP_LAST_XMM_REG(off, reg) \
    movaps off(%rsp), %reg;

#define POP_XMM_REG(off, reg, rest) \
    rest \
    POP_LAST_XMM_REG(off, reg)


/// used for counting the space needed to store all registers
#define PLUS_8_8 16
#define PLUS_8_16 24
#define PLUS_8_24 32
#define PLUS_8_32 40
#define PLUS_8_40 48
#define PLUS_8_48 56
#define PLUS_8_56 64
#define PLUS_8_64 72
#define PLUS_8_72 80
#define PLUS_8_80 88
#define PLUS_8_88 96
#define PLUS_8_96 104
#define PLUS_8_104 112
#define PLUS_8_112 120
#define PLUS_8_120 128
#define PLUS_8_128 136
#define PLUS_8_136 144
#define PLUS_8_144 152
#define PLUS_8_152 160

#define PLUS_EIGHT(_,rest) CAT(PLUS_8_, rest)
#define EIGHT(_) 8

/// used to save and restore registers
#define PUSHA ALL_REGS(PUSH_REG, PUSH_LAST_REG)
#define POPA ALL_REGS(POP_REG, POP_LAST_REG)
#define PUSHA_SIZE ALL_REGS(PLUS_EIGHT, EIGHT)

/// use to save and restore call-clobbered registers
#define PUSHA_CALL ALL_CALL_REGS(PUSH_REG, PUSH_LAST_REG)
#define POPA_CALL ALL_CALL_REGS(POP_REG, POP_LAST_REG)

/// used to save and restore xmm registers. Note: we assume that %rsp is
/// 16-byte aligned.
#define PUSHA_XMM \
    lea -256(%rsp), %rsp; \
    ALL_XMM_REGS(PUSH_XMM_REG, PUSH_LAST_XMM_REG)

#define POPA_XMM \
    ALL_XMM_REGS(POP_XMM_REG, POP_LAST_XMM_REG) \
    lea 256(%rsp), %rsp; \

#ifdef __APPLE__
#   define SHARED_SYMBOL(x) SYMBOL(x)(%rip)
#else // Linux
#   define SHARED_SYMBOL(x) SYMBOL(x)@PLT
#endif


/// Gives us a single register that isn't the
/// one inputted.
#define NOT_rax rcx
#define NOT_rcx rdx
#define NOT_rdx rbx
#define NOT_rbx rbp
#define NOT_rbp rsi
#define NOT_rsi rdi
#define NOT_rdi r8
#define NOT_r8 r9
#define NOT_r9 r10
#define NOT_r10 r11
#define NOT_r11 r12
#define NOT_r12 r13
#define NOT_r13 r14
#define NOT_r14 r15
#define NOT_r15 rax

/// Gives us a register that isn't RAX, if our current register is one of
/// the specified. This is convenient if we know that we need to modify RAX,
/// but don't know if our input register is RAX or not.
#define NOT_rax_rax rcx
#define NOT_rax_rcx rdx
#define NOT_rax_rdx rbx
#define NOT_rax_rbx rbp
#define NOT_rax_rbp rsi
#define NOT_rax_rsi rdi
#define NOT_rax_rdi r8
#define NOT_rax_r8 r9
#define NOT_rax_r9 r10
#define NOT_rax_r10 r11
#define NOT_rax_r11 r12
#define NOT_rax_r12 r13
#define NOT_rax_r13 r14
#define NOT_rax_r14 r15
#define NOT_rax_r15 rdx

/// Gives us a second register that isn't RAX, if our current register is one
/// of the specified, assuming we've already claimed a register according to
/// `NOT_rax_ ## reg`.
#define NOT_rax2_rax rdx
#define NOT_rax2_rcx rbx
#define NOT_rax2_rdx rbp
#define NOT_rax2_rbx rsi
#define NOT_rax2_rbp rsi
#define NOT_rax2_rsi r8
#define NOT_rax2_rdi r9
#define NOT_rax2_r8 r10
#define NOT_rax2_r9 r11
#define NOT_rax2_r10 r12
#define NOT_rax2_r11 r13
#define NOT_rax2_r12 r14
#define NOT_rax2_r13 r15
#define NOT_rax2_r14 rcx
#define NOT_rax2_r15 rbp

#define SPILL_REG__(reg) CAT(NOT_, reg)
#define SPILL_REG_(reg) SPILL_REG__(reg)
#define SPILL_REG(reg) SPILL_REG_(reg)

#define SPILL_REG_NOT_RAX__(reg) CAT(NOT_rax_, reg)
#define SPILL_REG_NOT_RAX_(reg) SPILL_REG_NOT_RAX__(reg)
#define SPILL_REG_NOT_RAX(reg) SPILL_REG_NOT_RAX_(reg)

#define REG64_TO_REG16_rax ax
#define REG64_TO_REG16_rcx cx
#define REG64_TO_REG16_rdx dx
#define REG64_TO_REG16_rbx bx
#define REG64_TO_REG16_rbp bp
#define REG64_TO_REG16_rsi si
#define REG64_TO_REG16_rdi di
#define REG64_TO_REG16_r8 r8w
#define REG64_TO_REG16_r9 r9w
#define REG64_TO_REG16_r10 r10w
#define REG64_TO_REG16_r11 r11w
#define REG64_TO_REG16_r12 r12w
#define REG64_TO_REG16_r13 r13w
#define REG64_TO_REG16_r14 r14w
#define REG64_TO_REG16_r15 r15w

#define REG64_TO_REG16__(reg) CAT(REG64_TO_REG16_, reg)
#define REG64_TO_REG16_(reg) REG64_TO_REG16__(reg)
#define REG64_TO_REG16(reg) REG64_TO_REG16_(reg)


/// Useful for documentation
#define COMMENT_HASH #
#define COMMENT(...) COMMENT_HASH __VA_ARGS__ @N@

#endif /* Granary_ASM_HELPERS_ASM_ */
