/*
 * asm_helpers.asm
 *
 *  Created on: 2012-11-27
 *      Author: pag
 *     Version: $Id$
 */

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

#define ADD_(x, y) x ## y
#define ADD(x, y) ADD_(x, y)
#define PLUS_EIGHT(_,rest) ADD(PLUS_8_, rest)
#define EIGHT(_) 8

/// used to save and restore registers
#define PUSHA ALL_REGS(PUSH_REG, PUSH_LAST_REG)
#define POPA ALL_REGS(POP_REG, POP_LAST_REG)
#define PUSHA_SIZE TO_STRING(ALL_REGS(PLUS_EIGHT, EIGHT))

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

#endif /* Granary_ASM_HELPERS_ASM_ */
