/*
 * shadow_update.asm
 *
 *  Created on: 2013-08-05
 *      Author: akshayk
 */


#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

#include "clients/watchpoints/config.h"


START_FILE

    .extern SYMBOL(_ZN6client2wp11DESCRIPTORSE)

#define SHADOW_OFFSET_READ  0x0
#define SHADOW_OFFSET_WRITE 0x8
#define SHADOW_OFFSET_BASE  0xd
#define WP_INDEX_WIDTH 16
#define ACCESS_FLAG_OFFSET 0x10

/// Watchpoint descriptor.
#define DESC(reg) SPILL_REG_NOT_RAX(reg)

/// Table address, as well as temporary for calculating the descriptor index.
#define TABLE(reg) SPILL_REG_NOT_RAX(DESC(reg))

/// unwatched address
#define ORIG_ADDR(reg) SPILL_REG_NOT_RAX(TABLE(reg))

#define BASE_ADDR(reg) SPILL_REG_NOT_RAX(ORIG_ADDR(reg))

#define READ_SHD(reg) SPILL_REG_NOT_RAX(BASE_ADDR(reg))

#define WRITE_SHD(reg) SPILL_REG_NOT_RAX(BASE_ADDR(reg))


    /// Define a register-specific function to mark a descriptor as having
    /// been accessed.
#define DESCRIPTOR_READ_ACCESSOR(reg) \
    DECLARE_FUNC(CAT(granary_shadow_update_read_, reg)) @N@\
    GLOBAL_LABEL(CAT(granary_shadow_update_read_, reg):) @N@@N@\
        COMMENT(Save the flags.) \
        pushf;@N@\
        \
        COMMENT(Spill some registers. The DEST and TABLE registers are) \
        COMMENT(guaranteed to not be RAX. We unconditionally spill RAX.) \
        pushq %rax;@N@\
        pushq %DESC(reg);@N@ \
        pushq %TABLE(reg);@N@ \
        @N@\
        \
        COMMENT(Save the watched address for indexing and in RAX) \
        mov %reg, %DESC(reg);@N@\
        IF_INHERITED_INDEX( mov %reg, %TABLE(reg);@N@ )\
        @N@\
        \
        COMMENT(Compute the combined index.) \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_LSH, %TABLE(reg);@N@ ) \
        IF_INHERITED_INDEX( shr $ WP_INHERITED_INDEX_RSH, %TABLE(reg);@N@ ) \
        shr $ WP_COUNTER_INDEX_RSH, %DESC(reg);@N@ \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_WIDTH, %DESC(reg);@N@ ) \
        IF_INHERITED_INDEX( or %TABLE(reg), %DESC(reg);@N@ ) \
        @N@\
        \
        COMMENT(Get the descriptor pointer from the index.) \
        lea SYMBOL(_ZN6client2wp11DESCRIPTORSE)(%rip), %TABLE(reg);@N@\
        lea (%TABLE(reg),%DESC(reg),8), %DESC(reg);@N@\
        \
        movq (%DESC(reg)), %DESC(reg);@N@\
        @N@\
        \
        COMMENT(Mark the descriptor as having been accessed.) \
        movb $1, ACCESS_FLAG_OFFSET(%DESC(reg));@N@\
        pushq %ORIG_ADDR(reg);@N@\
        movq %reg, %ORIG_ADDR(reg);@N@\
        sal $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        sar $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        pushq %BASE_ADDR(reg);@N@\
        movq SHADOW_OFFSET_BASE(%DESC(reg)), %BASE_ADDR(reg);@N@\
        subq %ORIG_ADDR(reg), %BASE_ADDR(reg);@N@\
        pushq %READ_SHD(reg);@N@\
        movq SHADOW_OFFSET_READ(%DESC(reg)), %READ_SHD(reg);@N@\
        bts %BASE_ADDR(reg), %READ_SHD(reg);@N@\
        \
        COMMENT(Unspill.) \
        popq %READ_SHD(reg);@N@\
        popq %BASE_ADDR(reg);@N@\
        popq %ORIG_ADDR(reg);@N@\
        popq %TABLE(reg);@N@\
        popq %DESC(reg);@N@\
        popq %rax;@N@\
        \
        COMMENT(Restore the flags.) \
        popf;@N@\
        @N@\
        ret;@N@\
    END_FUNC(CAT(granary_shadow_update_read_, reg)) @N@@N@@N@


#define DESCRIPTOR_WRITE_ACCESSOR(reg) \
    DECLARE_FUNC(CAT(granary_shadow_update_write_, reg)) @N@\
    GLOBAL_LABEL(CAT(granary_shadow_update_write_, reg):) @N@@N@\
        COMMENT(Save the flags.) \
        pushf;@N@\
        \
        COMMENT(Spill some registers. The DEST and TABLE registers are) \
        COMMENT(guaranteed to not be RAX. We unconditionally spill RAX.) \
        pushq %rax;@N@\
        pushq %DESC(reg);@N@ \
        pushq %TABLE(reg);@N@ \
        @N@\
        \
        COMMENT(Save the watched address for indexing and in RAX) \
        mov %reg, %DESC(reg);@N@\
        IF_INHERITED_INDEX( mov %reg, %TABLE(reg);@N@ )\
        @N@\
        \
        COMMENT(Compute the combined index.) \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_LSH, %TABLE(reg);@N@ ) \
        IF_INHERITED_INDEX( shr $ WP_INHERITED_INDEX_RSH, %TABLE(reg);@N@ ) \
        shr $ WP_COUNTER_INDEX_RSH, %DESC(reg);@N@ \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_WIDTH, %DESC(reg);@N@ ) \
        IF_INHERITED_INDEX( or %TABLE(reg), %DESC(reg);@N@ ) \
        @N@\
        \
        COMMENT(Get the descriptor pointer from the index.) \
        lea SYMBOL(_ZN6client2wp11DESCRIPTORSE)(%rip), %TABLE(reg);@N@\
        lea (%TABLE(reg),%DESC(reg),8), %DESC(reg);@N@\
        \
        movq (%DESC(reg)), %DESC(reg);@N@\
        @N@\
        \
        COMMENT(Mark the descriptor as having been accessed.) \
        movb $1, ACCESS_FLAG_OFFSET(%DESC(reg));@N@\
        pushq %ORIG_ADDR(reg);@N@\
        movq %reg, %ORIG_ADDR(reg);@N@\
        sal $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        sar $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        pushq %BASE_ADDR(reg);@N@\
        movq SHADOW_OFFSET_BASE(%DESC(reg)), %BASE_ADDR(reg);@N@\
        subq %ORIG_ADDR(reg), %BASE_ADDR(reg);@N@\
        pushq %READ_SHD(reg);@N@\
        movq SHADOW_OFFSET_WRITE(%DESC(reg)), %READ_SHD(reg);@N@\
        bts %BASE_ADDR(reg), %READ_SHD(reg);@N@\
        \
        COMMENT(Unspill.) \
        popq %READ_SHD(reg);@N@\
        popq %BASE_ADDR(reg);@N@\
        popq %ORIG_ADDR(reg);@N@\
        popq %TABLE(reg);@N@\
        popq %DESC(reg);@N@\
        popq %rax;@N@\
        \
        COMMENT(Restore the flags.) \
        popf;@N@\
        @N@\
        ret;@N@\
    END_FUNC(CAT(granary_shadow_update_write_, reg)) @N@@N@@N@

#define DESCRIPTOR_READ_ACCESSORS(reg, rest) \
    DESCRIPTOR_READ_ACCESSOR(reg) \
    rest

#define DESCRIPTOR_WRITE_ACCESSORS(reg, rest) \
    DESCRIPTOR_WRITE_ACCESSOR(reg) \
    rest


ALL_REGS(DESCRIPTOR_READ_ACCESSORS, DESCRIPTOR_READ_ACCESSOR)
ALL_REGS(DESCRIPTOR_WRITE_ACCESSORS, DESCRIPTOR_WRITE_ACCESSOR)

END_FILE
