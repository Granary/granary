/*
 * globals.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_GLOBALS_H_
#define granary_GLOBALS_H_

/// guard on including the standard library so that its included types don't
/// interact with the generated types of user/kernel_types in detach.cc.
#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include <cstring>
#   include <cstddef>
#   include <stdint.h>
#endif

#include "granary/types/dynamorio.h"
#include "granary/pp.h"


/// Set to 1 iff jumps that keep control within the same basic block should be
/// patched to jump directly back into the same basic block instead of being
/// turned into slot-based direct jump lookups.
#define CONFIG_BB_PATCH_LOCAL_BRANCHES 1


/// Set to 1 iff basic blocks should contain the instructions immediately
/// following a conditional branch. If enabled, basic blocks will be bigger.
#define CONFIG_BB_EXTEND_BBS_PAST_CBRS 0


/// Upper bound on the number of processor cores.
#ifndef CONFIG_MAX_NUM_CPUS
#   define CONFIG_MAX_NUM_CPUS 8
#endif


/// Lower bound on the cache line size.
#ifndef CONFIG_MIN_CACHE_LINE_SIZE
#   define CONFIG_MIN_CACHE_LINE_SIZE 64
#endif


/// Exact size of memory pages.
#ifndef CONFIG_MEMORY_PAGE_SIZE
#   define CONFIG_MEMORY_PAGE_SIZE 4096
#endif


/// The maximum wrapping depth for argument wrappers.
#ifndef CONFIG_MAX_PRE_WRAP_DEPTH
#   define CONFIG_MAX_PRE_WRAP_DEPTH 3
#endif
#ifndef CONFIG_MAX_POST_WRAP_DEPTH
#   define CONFIG_MAX_POST_WRAP_DEPTH 3
#endif
#ifndef CONFIG_MAX_RETURN_WRAP_DEPTH
#   define CONFIG_MAX_RETURN_WRAP_DEPTH 3
#endif


/// Translate `%rip`-relative addresses to absolute addresses in user space.
/// On some 64-bit systems (e.g. Max OS X), the heap tends to be located > 4GB
/// away from the memory region that contains the code. As a result, translated
/// `%rip`-relative addresses cannot fit in 32-bits.
#ifndef CONFIG_TRANSLATE_FAR_ADDRESSES
#   define CONFIG_TRANSLATE_FAR_ADDRESSES !GRANARY_IN_KERNEL
#endif

namespace granary {

    /// Program counter type.
    typedef dynamorio::app_pc app_pc;


    enum {

        /// Size in bytes of each memory page.
        PAGE_SIZE = CONFIG_MEMORY_PAGE_SIZE,


        /// Some non-zero positive multiple of the cache line size.
        CACHE_LINE_SIZE = CONFIG_MIN_CACHE_LINE_SIZE,


        /// number of processor cores.
        NUM_CPUS = CONFIG_MAX_NUM_CPUS,


        /// Maximum wrapping depths
        MAX_PRE_WRAP_DEPTH = CONFIG_MAX_PRE_WRAP_DEPTH,
        MAX_POST_WRAP_DEPTH = CONFIG_MAX_POST_WRAP_DEPTH,
        MAX_RETURN_WRAP_DEPTH = CONFIG_MAX_RETURN_WRAP_DEPTH
    };

    enum {

        /// Bounds on where kernel module code is placed
        KERNEL_MODULE_START = 0xffffffffa0000000ULL,
        KERNEL_MODULE_END = 0xfffffffffff00000ULL
    };


    /// Processor flags
    typedef unsigned long long flags_t;


    /// Forward declarations.
    struct basic_block;
}


extern "C" {
#if !GRANARY_IN_KERNEL
    extern void granary_break_on_fault(void);
    extern int granary_fault(void);

    extern void granary_break_on_encode(dynamorio::app_pc pc,
                                        dynamorio::instr_t *instr);
    extern void granary_break_on_bb(granary::basic_block *bb);
    extern void granary_break_on_allocate(void *ptr);
    extern int granary_test_return_true(void);
    extern int granary_test_return_false(void);
#else

    extern flags_t granary_disable_interrupts(void);
    extern void granary_restore_flags(flags_t);

#endif


    /// Get the APIC ID of the current processor.
    extern int granary_asm_apic_id(void);


    /// Perform an 8-byte atomic write.
    extern void granary_atomic_write8(uint64_t, uint64_t *);


    /// Get the current stack pointer.
    extern uint64_t granary_get_stack_pointer(void);
}


#include "granary/allocator.h"
#include "granary/utils.h"
#include "granary/type_traits.h"
#include "granary/bump_allocator.h"
#include "granary/init.h"
#include "granary/test.h"

#endif /* granary_GLOBALS_H_ */
