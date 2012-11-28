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


#endif /* Granary_ASM_HELPERS_ASM_ */
