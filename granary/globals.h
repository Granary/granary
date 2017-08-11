/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * globals.h
 *
 *      Author: Peter Goodman
 */

#ifndef granary_GLOBALS_H_
#define granary_GLOBALS_H_

/// guard on including the standard library so that its included types don't
/// interact with the generated types of user/kernel_types in detach.cc.
#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include <cstddef>
#   include <new>
#   include <stdint.h>
#endif


#ifndef GRANARY
#   define GRANARY 1
#endif


#ifndef IN_GRANARY_CODE
#   define IN_GRANARY_CODE
#endif


#ifndef CONFIG_ENV_KERNEL
#   define CONFIG_ENV_KERNEL 0
#endif


#ifndef GRANARY_USE_PIC
#   define GRANARY_USE_PIC 1
#endif


#ifndef GRANARY_WHOLE_KERNEL
#   define GRANARY_WHOLE_KERNEL 0
#endif


/// Should the host be instrument as well as the app?
#if CONFIG_ENV_KERNEL
#   define CONFIG_FEATURE_INSTRUMENT_HOST GRANARY_WHOLE_KERNEL
#else
#   define CONFIG_FEATURE_INSTRUMENT_HOST 1 // TODO: Perhaps default to 1.
#endif


/// Should Granary interpose on any interrupts? If this is disabled then any
/// faults caused by the kernel invoking unwrapped module code will not be
/// handled and will likely result in a crash. However, the benefit of disabling
/// this is that sometimes Granary introduces instability because of how it
/// handles interrupts. This instability can be resolved by not letting Granary
/// handle interrupts. Another potential benefit is decreased interrupt
/// latency because Granary won't add in additional code on each interrupt
/// invocation.
#if CONFIG_ENV_KERNEL
#   define CONFIG_FEATURE_HANDLE_INTERRUPTS 1 // shouldn't be 0, but can be sometimes.
#else
#   define CONFIG_FEATURE_HANDLE_INTERRUPTS 0 // can't change in user space
#endif


/// Can client code handle interrupts? This is needed for things like kernel-
/// space watchpoints clients (where we need to recover from a GP fault in
/// native code).
#if CONFIG_ENV_KERNEL
#   define CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT 1
#else
#   define CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT 0 // can't change in user space
#endif


/// Should we support interrupt delaying? Combined with clients handling
/// interrupts, this will affect performance because if both are disabled then
/// Granary will mostly get out of the way
#if CONFIG_ENV_KERNEL
#   define CONFIG_FEATURE_INTERRUPT_DELAY 0
#else
#   define CONFIG_FEATURE_INTERRUPT_DELAY 0 // can't change in user space
#endif


/// Should Granary double check instruction encodings? If enabled, this will
/// decode every encoded instruction to double check that the DynamoRIO side of
/// things is doing something sane and that some illegal operands weren't passed
/// to the DynamoRIO side of things.
#define CONFIG_DEBUG_CHECK_INSTRUCTION_ENCODE 0


/// Should Granary double check that any time CPU private data is accessed, that
/// interrupts are disabled?
#define CONFIG_DEBUG_CHECK_CPU_ACCESS_SAFE 0


/// If one is experiencing triple faults / spurious CPU rests, they might be
/// because of `kernel_get_cpu_state`, i.e. the `mov %gs:offset, reg` faults,
/// which goes into an interrupt handler which tries the same thing over again,
/// which faults again, etc. The current mechanism for debugging this problem
/// assumes that Granary is compiled with frame pointers.
#define CONFIG_DEBUG_CPU_RESET 0


/// Should the direct return optimisation be enabled? This is not available for
/// user space code; however, can make a different in kernel space.
#define CONFIG_OPTIMISE_DIRECT_RETURN CONFIG_ENV_KERNEL


/// Should profile guided optimisation be enabled? This can / should only be
/// toggled from the Makefile when a profile file is supplied to the
/// `GR_PGO_PROFILE` command-line argument.
#ifndef CONFIG_OPTIMISE_PGO
#   define CONFIG_OPTIMISE_PGO 0 // can't change.
#endif


/// Should execution be traced? This is a debugging option, not to be confused
/// with the trace allocator or trace building, where we record the entry PCs
/// of basic blocks as they execute for later inspection by gdb.
#define CONFIG_DEBUG_TRACE_EXECUTION 0
#define CONFIG_DEBUG_TRACE_PRINT_LOG 0
#define CONFIG_DEBUG_TRACE_RECORD_REGS 1
#define CONFIG_DEBUG_NUM_TRACE_LOG_ENTRIES 1024


/// Do pre-mangling of instructions with the REP prefix?
#define CONFIG_PRE_MANGLE_REP_INSTRUCTIONS 0


/// Enable the trace allocator? The trace allocator tries to approximate trace
/// building by having a basic block fragment allocated in the same slab (if
/// possible) as its successor basic block.
#if CONFIG_ENV_KERNEL
#   define CONFIG_ENABLE_TRACE_ALLOCATOR 0
#else
#   define CONFIG_ENABLE_TRACE_ALLOCATOR 0 // Can't change.
#endif


/// Optional trace allocator sub-option: Should all syscall entrypoints be
/// treated as distinct traces?
#define CONFIG_TRACE_ALLOCATE_ENTRY_SYSCALL 0


/// Optional trace allocator sub-option, which can be combined with other
/// sub-options: Should we also isolate the memory management and scheduler
/// code into their own trace allocators?
///
/// Note: These cannot be combined with functional unit tracing.
#define CONFIG_TRACE_ALLOCATE_MM 0
#define CONFIG_TRACE_ALLOCATE_SCHEDULE 0


/// Optional trace allocator sub-option: Should all functional units be treated
/// as distinct traces? This results in code being grouped into functions.
#define CONFIG_TRACE_ALLOCATE_FUNCTIONAL_UNITS 0


/// Optional trace allocator sub-option: Should the trace allocator be based on
/// CPU allocators? This results in traces with respect to CPU allocators.
///
/// Note: If the trace allocator is turned on that none of the other options
///       are set, then the implementation behavior is such that this is the
///       default.
#define CONFIG_TRACE_ALLOCATE_ENTRY_CPU 0


/// Should we do a delayed takeover of the kernel table? This is only relevant
/// for whole-kernel instrumentation.
///
/// Note: Disable with 0, or enable by setting N >= 1 as the number of
///       milliseconds to pause between takeover periods.
#define CONFIG_DELAYED_TAKEOVER 0


/// Should we try to aggressively build traces at basic block translation
/// time? This implies a more transactional approach to filling the code cache.
///
/// Note: This is unrelated to tracing execution for debugging, and unrelated
///       to trace allocators.
///
/// Note: This is a very very aggressive translation approach, and might
///       perform badly with policies.
///
/// Note: If a non-zero number is given, then that number represents the maximum
///       number of conditional branch fall-throughs to follow.
#define CONFIG_FOLLOW_FALL_THROUGH_BRANCHES 8


/// If we're following fall-through branches, then this option lets us also
/// follow conditional branches.
///
/// Note: This doesn't really work :-S
#define CONFIG_FOLLOW_CONDITIONAL_BRANCHES 0


/// Enable performance counters and reporting. Performance counters measure
/// things like number of translated bytes, number of code cache bytes, etc.
/// These counters allow us to get a sense of how (in)efficient Granary is with
/// memory, etc.
#define CONFIG_DEBUG_PERF_COUNTS 1


/// Debug the initialisation of Granary, but make sure that it doesn't actually
/// take over anything.
#define CONFIG_DEBUG_INITIALISE 0


/// Enable wrappers. If wrappers are enabled, then Granary will automatically
/// detach when certain functions are called. When Granary detaches, it will
/// sometimes call a "wrapped" version of the intended function, which may or
/// may not call the origin function. Because wrappers detach Granary, it is
/// important that the return address is a code cache address (or code cache
/// equivalent), so that Granary can regain control.
///
/// Note: Currently, Granary does not support wrappers with transparent return
///       addresses, partly due to its inability to regain control in some
///       circumstance (which is addressable) and partly because of its
///       inability to regain control in the proper policy.
#if CONFIG_ENV_KERNEL
#   define CONFIG_FEATURE_WRAPPERS 0
#else
#   define CONFIG_FEATURE_WRAPPERS 1 // Don't change?
#endif


/// The maximum wrapping depth for argument wrappers.
#ifndef CONFIG_MAX_PRE_WRAP_DEPTH
#   define CONFIG_MAX_PRE_WRAP_DEPTH 1
#endif
#ifndef CONFIG_MAX_POST_WRAP_DEPTH
#   define CONFIG_MAX_POST_WRAP_DEPTH 1
#endif
#ifndef CONFIG_MAX_RETURN_WRAP_DEPTH
#   define CONFIG_MAX_RETURN_WRAP_DEPTH 1
#endif


/// If we're using patch wrappers, the this will tell us whether or not we
/// should instrument the actual patch wrapper code, or just copy it whole-sale.
///
/// This is type of option comes up with full-kernel instrumentation and
/// watchpoints, where we might patch-wrap some code to wrap it, but at the
/// same time, it might try to de-reference watched memory, so we want to
/// make sure it doesn't fault.
#define CONFIG_FEATURE_INSTRUMENT_PATCH_WRAPPERS CONFIG_FEATURE_INSTRUMENT_HOST


/// Set the 1 iff we should run test cases (before doing anything else).
#define CONFIG_DEBUG_ASSERTIONS 1


#if CONFIG_ENV_KERNEL
#   define CONFIG_DEBUG_RUN_TEST_CASES 0 // don't change.
#else
#   define CONFIG_DEBUG_RUN_TEST_CASES (!GRANARY_USE_PIC && CONFIG_DEBUG_ASSERTIONS)
#endif


/// Lower bound on the cache line size.
///
/// If running on a relatively recent kernel version, then one should be able
/// to find the correct value using something like:
///     cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size
///
/// An alternative way of finding this value on Linux is to run:
///     getconf LEVEL1_DCACHE_LINESIZE
///
#ifndef CONFIG_ARCH_CACHE_LINE_SIZE
#   define CONFIG_ARCH_CACHE_LINE_SIZE 64
#endif


/// Exact size of memory pages.
#ifndef CONFIG_ARCH_PAGE_SIZE
#   define CONFIG_ARCH_PAGE_SIZE 4096
#endif


// Size of per-cpu/thread-local private stack (currently 6 pages).
#ifndef CONFIG_PRIVATE_STACK_SIZE
#   define CONFIG_PRIVATE_STACK_SIZE 24576
#endif


#include "granary/dynamorio.h"
#include "granary/pp.h"

namespace granary {


    /// Program counter type.
    typedef dynamorio::app_pc app_pc;
    typedef const unsigned char *const_app_pc;


    enum {

        /// Size in bytes of each memory page.
        PAGE_SIZE = CONFIG_ARCH_PAGE_SIZE,


        /// Some non-zero positive multiple of the cache line size.
        CACHE_LINE_SIZE = CONFIG_ARCH_CACHE_LINE_SIZE,


        /// Number of interrupt vectors
        NUM_INTERRUPT_VECTORS = 256,
        INTERRUPT_DELAY_CODE_SIZE = 2048,


        /// Size (in bytes) of the x86-64 user space redzone.
        REDZONE_SIZE = IF_USER_ELSE(128, 0),

        /// Maximum wrapping depths
        MAX_PRE_WRAP_DEPTH = CONFIG_MAX_PRE_WRAP_DEPTH,
        MAX_POST_WRAP_DEPTH = CONFIG_MAX_POST_WRAP_DEPTH,
        MAX_RETURN_WRAP_DEPTH = CONFIG_MAX_RETURN_WRAP_DEPTH,

        /// Number of bytes in *every* hot patched call. This allows us to say
        /// that a return address into code cache must always satisfy:
        ///                     (ret_addr % 8) == 5
        RETURN_ADDRESS_OFFSET = 5,

        __end_globals
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
            bool :1; // must be 1
            bool parity:1;
            bool :1; // must be 0
            bool aux_carry:1;
            bool :1; // must be 0
            bool zero:1;
            bool sign:1;
            bool trap:1;
            bool interrupt:1;
            bool direction:1;
            bool overflow:1;
            bool io_privilege_level:1;
            bool nested_task:1;
            bool :1; // must be 0
            bool resume:1;
            bool virtual_8086_mode:1;
            bool alignment_check:1;
            bool virtual_interrupt:1;
            bool virtual_interrupt_pending:1;
            bool id_flag:1;
            uint64_t :42;
        } __attribute__((packed));
        uint64_t value;

        inline void clear_arithmetic_flags(void) {
            carry = false;
            parity = false;
            aux_carry = false;
            zero = false;
            sign = false;
        }

    } __attribute__((packed));


    static_assert(8 == sizeof(eflags),
        "Invalid packing of `eflags` struct.");


    /// Detach from Granary.
    extern void detach(void) ;


    /// Detach context.
    enum runtime_context {
        RUNNING_AS_APP = 0,
        RUNNING_AS_HOST = 1
    };


    /// Forward declarations.
    struct basic_block;
    extern void run_tests(void) ;

#if CONFIG_ENV_KERNEL
    template <typename T>
    FORCE_INLINE
    void construct_object(T &obj) {
        new (&obj) T;
    }
#endif /* CONFIG_ENV_KERNEL */


    extern bool is_code_cache_address(const const_app_pc) ;
    extern bool is_wrapper_address(const const_app_pc) ;
    extern bool is_gencode_address(const const_app_pc) ;


#if CONFIG_ENV_KERNEL

    extern "C" bool is_host_address(uintptr_t addr) ;
    extern "C" bool is_app_address(uintptr_t addr) ;

    template <typename T>
    inline bool is_host_address(T *addr_) {
        return is_host_address(reinterpret_cast<uintptr_t>(addr_));
    }

    template <typename T>
    inline bool is_app_address(T *addr_) {
        return is_app_address(reinterpret_cast<uintptr_t>(addr_));
    }

#else
    extern app_pc find_detach_target(app_pc pc, runtime_context) ;

    template <typename T>
    inline bool is_host_address(T *addr_) {
        const uint64_t addr(reinterpret_cast<uint64_t>(addr_));
        return nullptr != find_detach_target(
            reinterpret_cast<app_pc>(addr), RUNNING_AS_APP);
    }

    template <typename T>
    inline bool is_app_address(T *) {
        return true; // TODO
    }

#endif /* CONFIG_ENV_KERNEL */
}


extern "C" {

    extern void granary_break_on_fault(void);
    extern void granary_break_on_curiosity(void);
    extern int granary_fault(void);

    extern granary::eflags granary_disable_interrupts(void);
    extern granary::eflags granary_load_flags(void);
    extern void granary_store_flags(granary::eflags);


    extern void *granary_memcpy(void *, const void *, size_t);
    extern void *granary_memset(void *, int, size_t);
    extern int granary_memcmp(const void *, const void *, size_t);
    extern size_t granary_strlen(const char *);
    extern char *granary_strncpy(char *destination, const char *source, size_t num);


#   define memcpy granary_memcpy
#   define memset granary_memset
#   define memcmp granary_memcmp
#   define strlen granary_strlen
#   define strncpy granary_strncpy

    extern bool granary_try_access(const void *);

#if CONFIG_ENV_KERNEL
    extern bool granary_fail_access(void);
    extern void kernel_log(const char *, size_t);
#endif /* CONFIG_ENV_KERNEL */

#if CONFIG_DEBUG_RUN_TEST_CASES
    extern int granary_test_return_true(void);
    extern int granary_test_return_false(void);
#endif /* CONFIG_DEBUG_RUN_TEST_CASES */


    /// Used for switching to a CPU-private stack.
    extern void granary_enter_private_stack(void);
    extern void granary_exit_private_stack(void);


#if CONFIG_DEBUG_ASSERTIONS
    /// Used in conjunction with GDB to improve code cache debugging.
    extern bool granary_do_break_on_translate;
    extern void granary_break_on_translate(void *addr);
#endif /* CONFIG_DEBUG_ASSERTIONS*/
}


#include "granary/allocator.h"
#include "granary/utils.h"
#include "granary/init.h"
#include "granary/perf.h"
#include "granary/printf.h"
#include "granary/trace_log.h"


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include "granary/bump_allocator.h"
#   include "granary/kernel/interrupt.h"
#if CONFIG_FEATURE_INSTRUMENT_HOST && CONFIG_ENV_KERNEL
#       include "granary/kernel/syscall.h"
#endif
#endif


namespace granary {
    /// Log some data from Granary to the external world.
    inline void log(const char *data, size_t IF_KERNEL( size ) ) {
        IF_KERNEL(kernel_log(data, size);)
        IF_USER(granary::printf("%s", data);)
    }
}

#endif /* granary_GLOBALS_H_ */
