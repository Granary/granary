/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *      Author: Peter Goodman
 */

#ifndef granary_STATE_H_
#define granary_STATE_H_

#include "granary/globals.h"
#include "granary/hash_table.h"
#include "granary/cpu_code_cache.h"
#include "granary/state_handle.h"

#include "clients/state.h"

#if GRANARY_IN_KERNEL
#   include "deps/drk/segment_descriptor.h"
#endif

namespace granary {


    struct thread_state;
    struct cpu_state;
    struct stack_state;
    struct basic_block_state;
    struct basic_block;
    struct cpu_state_handle;
    struct thread_state_handle;
    struct instruction_list_mangler;
    struct interrupt_stack_frame;


    /// Notify that we're entering granary.
    void enter(cpu_state_handle cpu) throw();


    struct stack_state {
        char base[CONFIG_PRIVATE_STACK_SIZE];
        uint64_t top[0];
    } __attribute__((aligned (CONFIG_MEMORY_PAGE_SIZE), packed));


    extern "C" {
        uint64_t *granary_get_private_stack_top(void);
        void granary_enter_private_stack(void);
        void granary_exit_private_stack(void);
    }

    /// Information maintained by granary about each thread.
    struct thread_state : public client::thread_state { };


    namespace detail {


        /// For small allocations.
        struct small_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = false,
                TRANSIENT = false,
                SHARED = false,
                EXEC_WHERE = EXEC_NONE,
                MIN_ALIGN = 1
            };
        };


        /// CPU-private fragment allocators.
        struct fragment_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE * 16,
                EXECUTABLE = true,
                TRANSIENT = false,
                SHARED = false,
                EXEC_WHERE = EXEC_CODE_CACHE,
                MIN_ALIGN = 16
            };
        };


        /// For transiently allocated instructions that are meant to appear
        /// "close enough" in memory with the rest of the code cache. This
        /// allocator feeds the DynamoRIO side of things.
        struct instruction_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = true,
                TRANSIENT = true,
                SHARED = false,
                EXEC_WHERE = EXEC_GEN_CODE,
                MIN_ALIGN = 1
            };
        };


        /// For dynamic wrapper entrypoints.
        struct wrapper_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = true,
                TRANSIENT = false,
                SHARED = true,
                EXEC_WHERE = EXEC_WRAPPER,
                MIN_ALIGN = 16
            };
        };


        /// CPU-private block-local storage allocators.
        struct block_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = false,
                TRANSIENT = false,
                SHARED = false,
                EXEC_WHERE = EXEC_NONE,
                MIN_ALIGN = 16
            };
        };


        /// Shared/gencode fragment allocators.
        struct global_fragment_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = true,
                TRANSIENT = false,
                SHARED = true,
                EXEC_WHERE = EXEC_GEN_CODE,
                MIN_ALIGN = 16
            };
        };


        /// Transient memory allocator.
        struct transient_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = false,
                TRANSIENT = true,
                SHARED = false,
                EXEC_WHERE = EXEC_NONE,
                MIN_ALIGN = 1
            };
        };
    }


#if GRANARY_IN_KERNEL
    namespace detail {
        struct interrupt_descriptor_table {
            descriptor_t vectors[
                 2 * NUM_INTERRUPT_VECTORS * sizeof(descriptor_t)];
        } __attribute__((aligned (CONFIG_MEMORY_PAGE_SIZE)));
    }
#endif


    /// Information maintained by granary about each CPU.
    ///
    /// Note: when in kernel space, we assume that this state
    /// 	  is only accessed with interrupts disabled.
    struct cpu_state : public client::cpu_state {
    public:

#if GRANARY_IN_KERNEL
#   if CONFIG_INSTRUMENT_HOST || CONFIG_HANDLE_INTERRUPTS

        /// Copy of this CPU's original interrupt descriptor table register
        /// value/
        system_table_register_t native_idtr;

        /// Granary's CPU-specific interrupt descriptor table.
        system_table_register_t idtr;

#if CONFIG_ENABLE_INTERRUPT_DELAY
        /// A region of executable code that is overwritten as necessary to
        /// delay interrupts.
        app_pc interrupt_delay_handler;
#endif

        /// Spilled registers needed for interrupt delaying.
        uint64_t spill[2];

#   endif
#   if CONFIG_INSTRUMENT_HOST

        /// Native value of the MSR_LSTAR model-specific register, which is
        /// normally used for system calls on this particular CPU.
        uint64_t native_msr_lstar;

        /// Granary's version of the MSR_LSTAR model-specific register, which
        /// is what is used for system calls on this CPU when Granary is
        /// instrumenting the whole kernel.
        uint64_t msr_lstar;
#   endif

        /// Linux-specific; bypass an exception table search if we think there's
        /// a place in this code that can legally access user space code.
        /// See `__do_page_fault` in the kernel source code, and search of
        /// `search_exception_tables`.
        void *last_exception_instruction_pointer;
        void *last_exception_table_entry;
#endif /* GRANARY_IN_KERNEL */


        /// Thread data.
        IF_USER( thread_state thread_data; )


        /// The code cache allocator for this CPU.
        bump_pointer_allocator<detail::small_allocator_config>
            small_allocator;


        /// The code cache allocator for this CPU.
        bump_pointer_allocator<detail::fragment_allocator_config>
            fragment_allocator;


        /// Allocator for instructions in process.
        bump_pointer_allocator<detail::instruction_allocator_config>
            instruction_allocator;


        /// The block-local storage allocator for this CPU.
        bump_pointer_allocator<detail::block_allocator_config>
            block_allocator;


        /// Allocator for objects whose lifetimes end before the next entry
        /// into Granary.
        bump_pointer_allocator<detail::transient_allocator_config>
            transient_allocator;


        /// The CPU-private "mirror" of the global code cache. This code cache
        /// mirrors the global one insofar as entries move from the global one
        /// into local ones over the course of execution.
        cpu_private_code_cache code_cache;


        /// A buffer, allocated from the global fragment allocator, that
        /// is used by DynamoRIO for privately encoding instructions.
        app_pc temp_instr_buffer;

        /// CPU-private stack.
        stack_state percpu_stack;


        /// Used by granary for early initialisation of the CPU state.
        IF_KERNEL( static void init_early(void) throw(); )
        IF_KERNEL( static void init_late(void) throw(); )
    };


    namespace detail {
        struct dummy_block_state : public client::basic_block_state {
            uint64_t __placeholder;
        };
    }


    /// Information maintained within each emitted basic block of
    /// translated application/module code.
    struct basic_block_state : public client::basic_block_state {
    private:

        friend struct instruction_list_mangler;
        friend struct code_cache;

    public:

        /// Returns the size of the basic block state. Because every struct has
        /// size at least 1 byte, we use this to determine if there *really* is
        /// anything in the basic block state, or if it's just an empty struct,
        /// in which case its size is actually 0 bytes.
        constexpr inline static unsigned size(void) throw() {
            return (sizeof(detail::dummy_block_state) > sizeof(uint64_t))
                ? sizeof(basic_block_state)
                : 0U;
        }
    };


    /// Global state.
    struct global_state {
    public:

        /// The fragment allocator for global gencode.
        static static_data<
            bump_pointer_allocator<detail::global_fragment_allocator_config>
        > FRAGMENT_ALLOCATOR;


        static static_data<
            bump_pointer_allocator<detail::wrapper_allocator_config>
        > WRAPPER_ALLOCATOR;
    };


    /// Client state.
    struct client_state { };
}

#endif /* granary_STATE_H_ */
