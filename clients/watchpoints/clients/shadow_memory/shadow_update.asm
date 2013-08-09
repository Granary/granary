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

#define SHADOW_OFFSET_READ      0
#define SHADOW_OFFSET_WRITE     8
#define SHADOW_OFFSET_BASE      24
#define WP_INDEX_WIDTH          16
#define ACCESS_FLAG_OFFSET      16

/// Watchpoint descriptor.
#define DESC(reg) SPILL_REG_NOT_RAX(reg)

/// Table address, as well as temporary for calculating the descriptor index.
#define TABLE(reg) SPILL_REG_NOT_RAX(DESC(reg))

/// unwatched address
#define ORIG_ADDR(reg) SPILL_REG_NOT_RAX(TABLE(reg))

#define BASE_ADDR(reg) SPILL_REG_NOT_RAX(ORIG_ADDR(reg))

#define INDEX_REG(reg)  SPILL_REG_NOT_RAX(BASE_ADDR(reg))

#define READ_SHD(reg) SPILL_REG_NOT_RAX(INDEX_REG(reg))

#define WRITE_SHD(reg) SPILL_REG_NOT_RAX(INDEX_REG(reg))



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
        push %rax;@N@\
        push %DESC(reg);@N@\
        push %TABLE(reg);@N@\
        @N@\
        \
        COMMENT(Save the watched address for indexing and in RAX) \
        mov %reg, %DESC(reg);@N@\
        shr $ WP_COUNTER_INDEX_RSH, %DESC(reg);@N@ \
        IF_INHERITED_INDEX( mov %reg, %TABLE(reg);@N@ )\
        @N@\
        \
        COMMENT(Compute the combined index.) \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_LSH, %TABLE(reg);@N@ ) \
        IF_INHERITED_INDEX( shr $ WP_INHERITED_INDEX_RSH, %TABLE(reg);@N@ ) \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_WIDTH, %DESC(reg);@N@ ) \
        IF_INHERITED_INDEX( or %TABLE(reg), %DESC(reg);@N@ ) \
        @N@\
        \
        COMMENT(Get the descriptor pointer from the index.) \
        lea SYMBOL(_ZN6client2wp11DESCRIPTORSE)(%rip), %TABLE(reg);@N@\
        movq (%TABLE(reg),%DESC(reg),8), %DESC(reg);@N@\
        @N@\
        \
        COMMENT(Mark the descriptor as having been accessed.) \
        movb $1, ACCESS_FLAG_OFFSET(%DESC(reg));@N@\
        pushq %BASE_ADDR(reg);@N@\
        @N@\
        \
        COMMENT(Get base address to calculate the shadow offset.) \
        movq SHADOW_OFFSET_BASE(%DESC(reg)), %BASE_ADDR(reg);@N@\
        @N@\
        \
        COMMENT(SIgn extend 48bit to 64bit base addresses.) \
        COMMENT(sal $ WP_INDEX_WIDTH, %BASE_ADDR(reg);)@N@\
        COMMENT(sar $ WP_INDEX_WIDTH, %BASE_ADDR(reg);)@N@\
        pushq %ORIG_ADDR(reg);@N@\
        movq %reg, %ORIG_ADDR(reg);@N@\
        @N@\
        \
        COMMENT(Sign extend and convert watched addresses to unwatched.) \
        sal $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        sar $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        \
        COMMENT(Get offset from base address.) \
        subq %BASE_ADDR(reg), %ORIG_ADDR(reg);@N@\
        pushq %INDEX_REG(reg);@N@\
        movq %ORIG_ADDR(reg), %INDEX_REG(reg);@N@\
        @N@\
        \
        COMMENT(Calculate the index and offset for bts operation.) \
        shr $0x6, %INDEX_REG(reg);@N@\
        and $0x3f, %ORIG_ADDR(reg);@N@\
        pushq %READ_SHD(reg);@N@\
        @N@\
        \
        COMMENT(Get read shadow address for updating bits.) \
        movq SHADOW_OFFSET_READ(%DESC(reg)), %READ_SHD(reg);@N@\
        bts %ORIG_ADDR(reg), (%READ_SHD(reg),%INDEX_REG(reg),8);@N@\
        popq %READ_SHD(reg);@N@\
        popq %INDEX_REG(reg);@N@\
        popq %ORIG_ADDR(reg);@N@\
        popq %BASE_ADDR(reg);@N@\
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
        push %rax;@N@\
        push %DESC(reg);@N@\
        push %TABLE(reg);@N@\
        @N@\
        \
        COMMENT(Save the watched address for indexing and in RAX) \
        mov %reg, %DESC(reg);@N@\
        shr $ WP_COUNTER_INDEX_RSH, %DESC(reg);@N@ \
        IF_INHERITED_INDEX( mov %reg, %TABLE(reg);@N@ )\
        @N@\
        \
        COMMENT(Compute the combined index.) \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_LSH, %TABLE(reg);@N@ ) \
        IF_INHERITED_INDEX( shr $ WP_INHERITED_INDEX_RSH, %TABLE(reg);@N@ ) \
        IF_INHERITED_INDEX( shl $ WP_INHERITED_INDEX_WIDTH, %DESC(reg);@N@ ) \
        IF_INHERITED_INDEX( or %TABLE(reg), %DESC(reg);@N@ ) \
        @N@\
        \
        COMMENT(Get the descriptor pointer from the index.) \
        lea SYMBOL(_ZN6client2wp11DESCRIPTORSE)(%rip), %TABLE(reg);@N@\
        movq (%TABLE(reg),%DESC(reg),8), %DESC(reg);@N@\
        \
        COMMENT(movq (%DESC(reg)), %DESC(reg);)@N@\
        @N@\
        \
        COMMENT(Mark the descriptor as having been accessed.) \
        movb $1, ACCESS_FLAG_OFFSET(%DESC(reg));@N@\
        pushq %BASE_ADDR(reg);@N@\
        @N@\
        \
        COMMENT(Get base address to calculate the shadow offset.) \
        movq SHADOW_OFFSET_BASE(%DESC(reg)), %BASE_ADDR(reg);@N@\
        @N@\
        \
        COMMENT(SIgn extend 48bit to 64bit base addresses.) \
        COMMENT(sal $ WP_INDEX_WIDTH, %BASE_ADDR(reg);)@N@\
        COMMENT(sar $ WP_INDEX_WIDTH, %BASE_ADDR(reg);)@N@\
        pushq %ORIG_ADDR(reg);@N@\
        movq %reg, %ORIG_ADDR(reg);@N@\
        @N@\
        \
        COMMENT(Sign extend and convert watched addresses to unwatched.) \
        sal $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        sar $ WP_INDEX_WIDTH, %ORIG_ADDR(reg);@N@\
        \
        COMMENT(Get offset from base address.) \
        subq %BASE_ADDR(reg), %ORIG_ADDR(reg);@N@\
        pushq %INDEX_REG(reg);@N@\
        movq %ORIG_ADDR(reg), %INDEX_REG(reg);@N@\
        @N@\
        \
        COMMENT(Calculate the index and offset for bts operation.) \
        shr $0x6, %INDEX_REG(reg);@N@\
        and $0x3f, %ORIG_ADDR(reg);@N@\
        pushq %WRITE_SHD(reg);@N@\
        @N@\
        \
        COMMENT(Get write shadow address for updating bits.) \
        movq SHADOW_OFFSET_WRITE(%DESC(reg)), %WRITE_SHD(reg);@N@\
        bts %ORIG_ADDR(reg), (%READ_SHD(reg),%INDEX_REG(reg),8);@N@\
        popq %WRITE_SHD(reg);@N@\
        popq %INDEX_REG(reg);@N@\
        popq %ORIG_ADDR(reg);@N@\
        popq %BASE_ADDR(reg);@N@\
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
