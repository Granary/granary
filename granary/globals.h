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


#ifndef GRANARY_IN_KERNEL
#   define GRANARY_IN_KERNEL 0
#endif


/// Enable IBL entry stubs. IBL entry stubs make use of a form of "branch
/// prediction" to try to reduce the cost of looking things up in the CPU
/// private hash table. If IBL entry stubs are not used, then generic IBL
/// entries are used.
#define CONFIG_ENABLE_IBL_PREDICTION_STUBS 1


/// Enable performance counters and reporting. Performance counters measure
/// things like number of translated bytes, number of code cache bytes, etc.
/// These counters allow us to get a sense of how (in)efficient Granary is with
/// memory, etc.
#define CONFIG_ENABLE_PERF_COUNTS 0


/// Enable wrappers. If wrappers are enabled, then Granary will automatically
/// detach when certain functions are called. When Granary detaches, it will
/// sometimes call a "wrapped" version of the intended function, which may or
/// may not call the origin function. Because wrappers detach Granary, it is
/// important that the return address is a code cache address (or code cache
/// equivalent), so that Granary can regain control.
///
/// Note: Currently, Granary does not support wrappers with transparent return
///       addresses, partly due to its inability to regain control in some
///       circumstance (which is addressable) and partly because of its inability
///       to regain control in the proper policy.
#define CONFIG_ENABLE_WRAPPERS 1


/// Enable transparent return addresses. This turns every function call into
/// an emulated function call that first pushes on a native return address and
/// then jmps to the destination. This option affects performance in a number of
/// ways. First, all return addresses are unconditionally lookup up in the IBL.
/// Second, extra trampolining mechanisms are used in order to emulate the
/// expected code cache policy behaviours.
#define CONFIG_TRANSPARENT_RETURN_ADDRESSES 0


/// Currently we disallow enabling both wrappers and transparent return addresses,
/// mainly because it is possible that we could get to a point where we can't
/// guarantee that we will be able to return into the code cache. The main
/// problem can be overcome more easily in kernel space, where we have the
/// expectation of fewer indirect jumps to wrappers (detach points), as well as
/// the ability to page protect the module against execution and recover when
/// and exception occurs.
#if CONFIG_ENABLE_WRAPPERS && CONFIG_TRANSPARENT_RETURN_ADDRESSES && !GRANARY_IN_KERNEL
#   error "Wrappers and transparent return addresses are not concurrently supported in user space."
#endif


/// Track usage of the SSE/SSE2 XMM register so that we can avoid saving and
/// restoring those registers.
#define CONFIG_TRACK_XMM_REGS 1


/// Save only the arithmetic flags instead of all flags when doing indirect
/// branch lookup. This only affects user space because in kernel space all
/// flags will be saved in order to disable interrupts.
#define CONFIG_IBL_SAVE_ALL_FLAGS 0


/// Use "precise" memory allocation, i.e. no pool allocators. This makes it
/// easier to find misuses of memory when Granary does something wrong (e.g.
/// buffer overflow within a slab).
#define CONFIG_PRECISE_ALLOCATE 0


/// Set the 1 iff we should run test cases (before doing anything else).
#ifdef GRANARY_USE_PIC
#   define CONFIG_RUN_TEST_CASES 0
#else
#   define CONFIG_RUN_TEST_CASES 1
#endif

/// Set to 1 iff jumps that keep control within the same basic block should be
/// patched to jump directly back into the same basic block instead of being
/// turned into slot-based direct jump lookups.
#define CONFIG_BB_PATCH_LOCAL_BRANCHES 1


/// Set to 1 iff basic blocks should contain the instructions immediately
/// following a conditional branch. If enabled, basic blocks will be bigger.
#define CONFIG_BB_EXTEND_BBS_PAST_CBRS 1


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

#include "granary/types/dynamorio.h"
#include "granary/pp.h"

namespace granary {

    /// Program counter type.
    typedef dynamorio::app_pc app_pc;


    enum {

        /// Size in bytes of each memory page.
        PAGE_SIZE = CONFIG_MEMORY_PAGE_SIZE,


        /// Some non-zero positive multiple of the cache line size.
        CACHE_LINE_SIZE = CONFIG_MIN_CACHE_LINE_SIZE,


        /// Size (in bytes) of the x86-64 user space redzone.
        REDZONE_SIZE = IF_USER_ELSE(128, 0),

        /// Maximum wrapping depths
        MAX_PRE_WRAP_DEPTH = CONFIG_MAX_PRE_WRAP_DEPTH,
        MAX_POST_WRAP_DEPTH = CONFIG_MAX_POST_WRAP_DEPTH,
        MAX_RETURN_WRAP_DEPTH = CONFIG_MAX_RETURN_WRAP_DEPTH,

#if CONFIG_ENABLE_IBL_PREDICTION_STUBS
        MAX_IBL_PREDICT_SLOTS = 4,
#endif

        /// Number of bytes in *every* hot patched call. This allows us to say
        /// that a return address into code cache must always satisfy:
        ///                     (ret_addr % 8) == 5
        RETURN_ADDRESS_OFFSET = 5,

        __end_globals
    };

    enum {

        /// Bounds on where kernel module code is placed
        KERNEL_MODULE_START = 0xffffffffa0000000ULL,
        KERNEL_MODULE_END = 0xfffffffffff00000ULL
    };


    /// Policy for storing a value in a hash table.
    enum hash_store_policy {
        HASH_OVERWRITE_PREV_ENTRY,
        HASH_KEEP_PREV_ENTRY
    };


    /// Store state.
    enum hash_store_state {
        HASH_ENTRY_STORED_NEW,
        HASH_ENTRY_STORED_OVERWRITE,
        HASH_ENTRY_SKIPPED
    };


    /// Processor flags
    typedef unsigned long long flags_t;


    /// Forward declarations.
    struct basic_block;

#if CONFIG_RUN_TEST_CASES
    extern void run_tests(void) throw();
#endif
}


extern "C" {
#if !GRANARY_IN_KERNEL
    extern void granary_break_on_fault(void);
    extern int granary_fault(void);

    extern void granary_break_on_encode(dynamorio::app_pc pc,
                                        dynamorio::instr_t *instr);
    extern void granary_break_on_bb(granary::basic_block *bb);
    extern void granary_break_on_allocate(void *ptr);
#   if CONFIG_RUN_TEST_CASES
    extern int granary_test_return_true(void);
    extern int granary_test_return_false(void);
#   endif
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
#include "granary/perf.h"
#include "granary/printf.h"

#endif /* granary_GLOBALS_H_ */
