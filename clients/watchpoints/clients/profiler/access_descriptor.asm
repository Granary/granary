/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * leak_policy.asm
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman
 */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"
#include "granary/pp.h"

#include "clients/watchpoints/config.h"


START_FILE

    .extern SYMBOL(_ZN6client2wp11DESCRIPTORSE)



#define APP_OFFSET_COUNTER      0
#define HOST_OFFSET_COUNTER     8

/// Watchpoint descriptor.
#define DESC(reg) SPILL_REG_NOT_RAX(reg)


/// Table address, as well as temporary for calculating the descriptor index.
#define TABLE(reg) SPILL_REG_NOT_RAX(DESC(reg))


    /// Define a register-specific function to mark a descriptor as having
    /// been accessed.
#define DESCRIPTOR_ACCESSOR_APP(reg) \
    DECLARE_FUNC(CAT(granary_access_descriptor_app_, reg)) @N@\
    GLOBAL_LABEL(CAT(granary_access_descriptor_app_, reg):) @N@@N@\
        COMMENT(Save the flags.) \
        pushf;@N@\
        \
        COMMENT(Spill some registers. The DEST and TABLE registers are) \
        COMMENT(guaranteed to not be RAX. We unconditionally spill RAX.) \
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
        addq $1, APP_OFFSET_COUNTER(%DESC(reg));@N@\
        \
        COMMENT(Unspill.) \
        popq %TABLE(reg);@N@\
        popq %DESC(reg);@N@\
        \
        COMMENT(Restore the flags.) \
        popf;@N@\
        @N@\
        ret;@N@\
    END_FUNC(CAT(granary_access_descriptor_app_, reg)) @N@@N@@N@


#define DESCRIPTOR_ACCESSOR_HOST(reg) \
    DECLARE_FUNC(CAT(granary_access_descriptor_host_, reg)) @N@\
    GLOBAL_LABEL(CAT(granary_access_descriptor_host_, reg):) @N@@N@\
        COMMENT(Save the flags.) \
        pushf;@N@\
        \
        COMMENT(Spill some registers. The DEST and TABLE registers are) \
        COMMENT(guaranteed to not be RAX. We unconditionally spill RAX.) \
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
        addq $1, HOST_OFFSET_COUNTER(%DESC(reg));@N@\
        \
        COMMENT(Unspill.) \
        popq %TABLE(reg);@N@\
        popq %DESC(reg);@N@\
        \
        COMMENT(Restore the flags.) \
        popf;@N@\
        @N@\
        ret;@N@\
    END_FUNC(CAT(granary_access_descriptor_host_, reg)) @N@@N@@N@




#define DESCRIPTOR_ACCESSORS_APP(reg, rest) \
    DESCRIPTOR_ACCESSOR_APP(reg) \
    rest

#define DESCRIPTOR_ACCESSORS_HOST(reg, rest) \
    DESCRIPTOR_ACCESSOR_HOST(reg) \
    rest


ALL_REGS(DESCRIPTOR_ACCESSORS_APP, DESCRIPTOR_ACCESSOR_APP)
ALL_REGS(DESCRIPTOR_ACCESSORS_HOST, DESCRIPTOR_ACCESSOR_HOST)

END_FILE
