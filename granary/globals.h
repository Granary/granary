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
#endif

#include <stdint.h>

#ifndef GRANARY
#   define GRANARY 1
#endif


#ifndef IN_GRANARY_CODE
#   define IN_GRANARY_CODE
#endif


#ifndef GRANARY_IN_KERNEL
#   define GRANARY_IN_KERNEL 1
#endif


#ifndef GRANARY_USE_PIC
#   define GRANARY_USE_PIC 0
#endif


#ifndef GRANARY_WHOLE_KERNEL
#   define GRANARY_WHOLE_KERNEL 0
#endif


/// Should Granary interpose on any interrupts? If this is disabled then any
/// faults caused by the kernel invoking unwrapped module code will not be
/// handled and will likely result in a crash. However, the benefit of disabling
/// this is that sometimes Granary introduces instability because of how it
/// handles interrupts. This instability can be resolved by not letting Granary
/// handle interrupts. Another potential benefit is decreased interrupt
/// latency because Granary won't add in additional code on each interrupt
/// invocation.
#if GRANARY_IN_KERNEL
#   define CONFIG_HANDLE_INTERRUPTS 1
#else
#   define CONFIG_HANDLE_INTERRUPTS 0 // can't change in user space
#endif


/// Should the host be instrument as well as the app?
#if GRANARY_IN_KERNEL
#   define CONFIG_INSTRUMENT_HOST GRANARY_WHOLE_KERNEL
#else
#   define CONFIG_INSTRUMENT_HOST 0 // TODO: Perhaps default to 1.
#endif


/// Can client code handle interrupts?
#if GRANARY_IN_KERNEL
#   define CONFIG_CLIENT_HANDLE_INTERRUPT 1
#else
#   define CONFIG_CLIENT_HANDLE_INTERRUPT 0 // can't change in user space
#endif


/// Should we support interrupt delaying? Combined with clients handling
/// interrupts, this will affect performance because if both are disabled then
/// Granary will mostly get out of the way
#if GRANARY_IN_KERNEL
#   define CONFIG_ENABLE_INTERRUPT_DELAY 0
#else
#   define CONFIG_ENABLE_INTERRUPT_DELAY 0 // can't change in user space
#endif


/// Should Granary double check instruction encodings? If enabled, this will
/// decode every encoded instruction to double check that the DynamoRIO side of
/// things is doing something sane and that some illegal operands weren't passed
/// to the DynamoRIO side of things.
#define CONFIG_CHECK_INSTRUCTION_ENCODE 0


/// Should Granary double check that any time CPU private data is accessed, that
/// interrupts are disabled?
#define CONFIG_CHECK_CPU_ACCESS_SAFE 0


/// If one is experiencing triple faults / spurious CPU rests, they might be
/// because of `kernel_get_cpu_state`, i.e. the `mov %gs:offset, reg` faults,
/// which goes into an interrupt handler which tries the same thing over again,
/// which faults again, etc. The current mechanism for debugging this problem
/// assumes that Granary is compiled with frame pointers.
#define CONFIG_DEBUG_CPU_RESET 0


/// Should the direct return optimisation be enabled? This is not available for
/// user space code; however, can make a different in kernel space.
#define CONFIG_ENABLE_DIRECT_RETURN GRANARY_IN_KERNEL


/// Should execution be traced?
#ifndef CONFIG_TRACE_EXECUTION
	#define CONFIG_TRACE_EXECUTION 0
#endif
#ifndef CONFIG_TRACE_PRINT_LOG
	#define CONFIG_TRACE_PRINT_LOG 0
#endif
#define CONFIG_TRACE_RECORD_REGS 1
#define CONFIG_NUM_TRACE_LOG_ENTRIES 1024


/// Do pre-mangling of instructions with the REP prefix?
#define CONFIG_PRE_MANGLE_REP_INSTRUCTIONS 0


/// Enable performance counters and reporting. Performance counters measure
/// things like number of translated bytes, number of code cache bytes, etc.
/// These counters allow us to get a sense of how (in)efficient Granary is with
/// memory, etc.
#define CONFIG_ENABLE_PERF_COUNTS 1


/// Enable profiling of indirect jumps and indirect calls.
#if CONFIG_ENABLE_PERF_COUNTS
#   define CONFIG_PROFILE_IBL 0
#else
#   define CONFIG_PROFILE_IBL 0 // can't change
#endif


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
#if GRANARY_IN_KERNEL
#   define CONFIG_ENABLE_WRAPPERS (!CONFIG_INSTRUMENT_HOST)
#else
#   define CONFIG_ENABLE_WRAPPERS 1 // can't change; not re-entrant with malloc.
#endif


/// If we're using patch wrappers, the this will tell us whether or not we
/// should instrument the actual patch wrapper code, or just copy it whole-sale.
///
/// This is type of option comes up with full-kernel instrumentation and
/// watchpoints, where we might patch-wrap some code to wrap it, but at the
/// same time, it might try to de-reference watched memory, so we want to
/// make sure it doesn't fault.
#define CONFIG_INSTRUMENT_PATCH_WRAPPERS CONFIG_INSTRUMENT_HOST


/// Set the 1 iff we should run test cases (before doing anything else).
#define CONFIG_ENABLE_ASSERTIONS 1
#if GRANARY_IN_KERNEL
#   define CONFIG_RUN_TEST_CASES 0 // don't change.
#else
#   define CONFIG_RUN_TEST_CASES (!GRANARY_USE_PIC && CONFIG_ENABLE_ASSERTIONS)
#endif


/// Lower bound on the cache line size.
#ifndef CONFIG_MIN_CACHE_LINE_SIZE
#   define CONFIG_MIN_CACHE_LINE_SIZE 64
#endif


/// Exact size of memory pages.
#ifndef CONFIG_MEMORY_PAGE_SIZE
#   define CONFIG_MEMORY_PAGE_SIZE 4096
#endif

// Size of per-cpu/thread-local private stack (currently 6 pages).
#ifndef CONFIG_PRIVATE_STACK_SIZE
#   define CONFIG_PRIVATE_STACK_SIZE 24576
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


#include "granary/dynamorio.h"
#include "granary/pp.h"

namespace granary {


    /// Program counter type.
    typedef dynamorio::app_pc app_pc;
    typedef const uint8_t *const_app_pc;


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

        inline void clear_arithmetic_flags(void) throw() {
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
    extern void detach(void) throw();


    /// Detach context.
    enum runtime_context {
        RUNNING_AS_APP = 0,
        RUNNING_AS_HOST = 1
    };


    /// Forward declarations.
    struct basic_block;


#if CONFIG_RUN_TEST_CASES
    extern void run_tests(void) throw();
#endif /* CONFIG_RUN_TEST_CASES */


#if GRANARY_IN_KERNEL
    template <typename T>
    FORCE_INLINE
    void construct_object(T &obj) throw() {
        new (&obj) T;
    }
#endif /* GRANARY_IN_KERNEL */


    extern "C" bool is_code_cache_address(app_pc) throw();
    extern "C" bool is_wrapper_address(app_pc) throw();
    extern "C" bool is_gencode_address(app_pc) throw();


#if GRANARY_IN_KERNEL

    extern "C" bool is_host_address(uintptr_t addr) throw();
    extern "C" bool is_app_address(uintptr_t addr) throw();

    template <typename T>
    inline bool is_host_address(T *addr_) throw() {
        return is_host_address(reinterpret_cast<uintptr_t>(addr_));
    }

    template <typename T>
    inline bool is_app_address(T *addr_) throw() {
        return is_app_address(reinterpret_cast<uintptr_t>(addr_));
    }

#else
    extern app_pc find_detach_target(app_pc pc, runtime_context) throw();

    template <typename T>
    inline bool is_host_address(T *addr_) throw() {
        const uint64_t addr(reinterpret_cast<uint64_t>(addr_));
        return nullptr != find_detach_target(
            reinterpret_cast<app_pc>(addr), RUNNING_AS_APP);
    }

    template <typename T>
    inline bool is_app_address(T *) throw() {
        return true; // TODO
    }

#endif /* GRANARY_IN_KERNEL */
}


extern "C" {

    extern void granary_break_on_fault(void);
    extern void granary_break_on_curiosity(void);
    extern int granary_fault(void);

    extern granary::eflags granary_disable_interrupts(void);
    extern granary::eflags granary_load_flags(void);
    extern void granary_store_flags(granary::eflags);

    extern uintptr_t granary_get_gs_base(void);
    extern uintptr_t granary_get_fs_base(void);

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

#if GRANARY_IN_KERNEL
    extern void kernel_log(const char *, size_t);
#endif /* GRANARY_IN_KERNEL */

#if CONFIG_RUN_TEST_CASES
    extern int granary_test_return_true(void);
    extern int granary_test_return_false(void);
#endif /* CONFIG_RUN_TEST_CASES */


    /// Perform an 8-byte atomic write.
    extern void granary_atomic_write8(uint64_t, uint64_t *);


    /// Get the current stack pointer.
    extern uint64_t granary_get_stack_pointer(void);


    /// Used for switching to a CPU-private stack.
    extern void granary_enter_private_stack(void);
    extern void granary_exit_private_stack(void);


#if CONFIG_ENABLE_ASSERTIONS
    /// Used in conjunction with GDB to improve code cache debugging.
    extern bool granary_do_break_on_translate;
    extern void granary_break_on_translate(void *addr);
#endif /* CONFIG_ENABLE_ASSERTIONS*/
}


#include "granary/allocator.h"
#include "granary/utils.h"
#include "granary/type_traits.h"
#include "granary/bump_allocator.h"
#include "granary/init.h"
#include "granary/perf.h"
#include "granary/printf.h"
#include "granary/trace_log.h"

#include "granary/kernel/interrupt.h"

#if CONFIG_INSTRUMENT_HOST && GRANARY_IN_KERNEL
#   include "granary/kernel/syscall.h"
#endif

namespace granary {
    /// Log some data from Granary to the external world.
    inline void log(const char *data, size_t size) throw() {
        USED(size);
        IF_KERNEL(kernel_log(data, size);)
        IF_USER(printf(data);)
    }
}

#endif /* granary_GLOBALS_H_ */
