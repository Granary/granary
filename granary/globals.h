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
#   include <new>
#endif

#include <stdint.h>

#ifndef GRANARY
#   define GRANARY 1
#endif


#ifndef GRANARY_IN_KERNEL
#   define GRANARY_IN_KERNEL 1
#endif


/// Can client code handle interrupts?
#if GRANARY_IN_KERNEL
#   define CONFIG_CLIENT_HANDLE_INTERRUPT 1
#else
#   define CONFIG_CLIENT_HANDLE_INTERRUPT 0
#endif


/// Should the direct return optimisation be enabled? This is not available for
/// user space code; however, can make a different in kernel space.
#if GRANARY_IN_KERNEL
#   define CONFIG_ENABLE_DIRECT_RETURN 1
#else
#   define CONFIG_ENABLE_DIRECT_RETURN 0 // can't change in user space
#endif


/// Use an RCU-protected hash table for the global code cache lookups, or use
/// a global lock?
#define CONFIG_LOCK_GLOBAL_CODE_CACHE 1


/// Is instrumentation using only one policy? If so, there are some optimisation
/// opportunities that involve looking for places where direct control-flow
/// transfers don't need to be dynamically resolved / hot patched if we have
/// already seen them.
#define CONFIG_USE_ONLY_ONE_POLICY 1


/// Enable IBL entry stubs. IBL entry stubs make use of a form of "branch
/// prediction" to try to reduce the cost of looking things up in the CPU
/// private hash table. If IBL entry stubs are not used, then generic IBL
/// entries are used.
#define CONFIG_ENABLE_IBL_PREDICTION_STUBS 1


#if CONFIG_ENABLE_IBL_PREDICTION_STUBS

    /// Should single overwrite prediction tables be used? If set to zero, then
    /// the prediction tables (if used) will default to starting with the next
    /// used prediction method.
#   define CONFIG_ENABLE_IBL_OVERWRITE_TABLE 1


    /// Should linear prediction tables be used?
#   define CONFIG_ENABLE_IBL_LINEAR_TABLE 1


#endif /* CONFIG_ENABLE_IBL_PREDICTION_STUBS */


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
#if GRANARY_IN_KERNEL
#   define CONFIG_TRACK_XMM_REGS 0
#else
#   define CONFIG_TRACK_XMM_REGS 1
#endif

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
#   define CONFIG_RUN_TEST_CASES 0
#endif


/// Set to 1 iff jumps that keep control within the same basic block should be
/// patched to jump directly back into the same basic block instead of being
/// turned into slot-based direct jump lookups.
#define CONFIG_BB_PATCH_LOCAL_BRANCHES 0


/// Set to 1 iff basic blocks should contain the instructions immediately
/// following a conditional branch. If enabled, basic blocks will be bigger.
#define CONFIG_BB_EXTEND_BBS_PAST_CBRS 0


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
#   define CONFIG_MAX_PRE_WRAP_DEPTH 2
#endif
#ifndef CONFIG_MAX_POST_WRAP_DEPTH
#   define CONFIG_MAX_POST_WRAP_DEPTH 2
#endif
#ifndef CONFIG_MAX_RETURN_WRAP_DEPTH
#   define CONFIG_MAX_RETURN_WRAP_DEPTH 2
#endif


/// Translate `%rip`-relative addresses to absolute addresses in user space.
/// On some 64-bit systems (e.g. Max OS X), the heap tends to be located > 4GB
/// away from the memory region that contains the code. As a result, translated
/// `%rip`-relative addresses cannot fit in 32-bits.
#ifndef CONFIG_TRANSLATE_FAR_ADDRESSES
#   define CONFIG_TRANSLATE_FAR_ADDRESSES 1
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


        /// Number of interrupt vectors
        NUM_INTERRUPT_VECTORS = 256,
        INTERRUPT_DELAY_CODE_SIZE = 2048,


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
        MODULE_TEXT_START = 0xffffffffa0000000ULL,
        MODULE_TEXT_END = 0xfffffffffff00000ULL,

        KERNEL_TEXT_START = 0xffffffff80000000ULL,
        KERNEL_TEXT_END = MODULE_TEXT_START
    };


    /// Kinds of executable memory. Must match the enum of the same name in
    /// module.c.
    enum executable_memory_kind {
        EXEC_CODE_CACHE = 0,
        EXEC_GEN_CODE = 1,
        EXEC_WRAPPER = 2,
        EXEC_NONE = 3
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


    /// Flags register structure.
    union eflags {
        struct {
            bool carry:1; // low
            bool _1:1; // must be 1
            bool parity:1;
            bool _3:1; // must be 0
            bool aux_carry:1;
            bool _5:1; // must be 0
            bool zero:1;
            bool sign:1;
            bool trap:1;
            bool interrupt:1;
            bool direction:1;
            bool overflow:1;
            bool io_privilege_level:1;
            bool nested_task:1;
            bool _15:1; // must be 0
            bool resume:1;
            bool virtual_8086_mode:1;
            bool alignment_check:1;
            bool virtual_interrupt:1;
            bool virtual_interrupt_pending:1;
            bool id_flag:1;
            uint32_t _22_to_31:10;
            uint32_t _32_to_63;
        } __attribute__((packed));
        uint64_t value;
    } __attribute__((packed));


    static_assert(8 == sizeof(eflags),
        "Invalid packing of `eflags` struct.");


    /// Forward declarations.
    struct basic_block;


#if CONFIG_RUN_TEST_CASES
    extern void run_tests(void) throw();
#endif


#if GRANARY_IN_KERNEL
    template <typename T>
    FORCE_INLINE
    void construct_object(T &obj) throw() {
        new (&obj) T;
    }
#endif


    extern bool is_code_cache_address(app_pc) throw();
    extern bool is_wrapper_address(app_pc) throw();


#if GRANARY_IN_KERNEL
    template <typename T>
    inline bool is_host_address(T *addr_) throw() {
        const uint64_t addr(reinterpret_cast<uint64_t>(addr_));
        return KERNEL_TEXT_START <= addr && addr < KERNEL_TEXT_END;
    }

    template <typename T>
    inline bool is_app_address(T *addr_) throw() {
        const uint64_t addr(reinterpret_cast<uint64_t>(addr_));
        if(MODULE_TEXT_START <= addr && addr < MODULE_TEXT_END) {
            return !is_code_cache_address(reinterpret_cast<app_pc>(addr));
        }
        return false;
    }

#else
    template <typename T>
    inline bool is_host_address(T *addr_) throw() {
        extern app_pc find_detach_target(app_pc pc) throw();
        const uint64_t addr(reinterpret_cast<uint64_t>(addr_));
        return nullptr != find_detach_target(
            reinterpret_cast<app_pc>(addr));
    }

    template <typename T>
    inline bool is_app_address(T *) throw() {
        return true; // TODO
    }

#endif
}


extern "C" {

    extern void granary_break_on_fault(void);
    extern int granary_fault(void);

#if GRANARY_IN_KERNEL
    extern unsigned long long granary_disable_interrupts(void);
    extern void granary_restore_flags(unsigned long long);
    extern void kernel_preempt_disable(void);
    extern void kernel_preempt_enable(void);
#endif

#if CONFIG_RUN_TEST_CASES
    extern int granary_test_return_true(void);
    extern int granary_test_return_false(void);
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

#if CONFIG_CLIENT_HANDLE_INTERRUPT
#   include "granary/kernel/interrupt.h"
#endif

#endif /* granary_GLOBALS_H_ */
