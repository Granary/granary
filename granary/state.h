/*
 * state.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_STATE_H_
#define granary_STATE_H_

#include "granary/globals.h"
#include "granary/hash_table.h"
#include "granary/cpu_code_cache.h"

#include "clients/state.h"

namespace granary {


    struct thread_state;
    struct cpu_state;
    struct basic_block_state;
    struct basic_block;
    struct cpu_state_handle;
    struct thread_state_handle;
    struct instruction_list_mangler;


    /// Notify that we're entering granary.
    void enter(cpu_state_handle &cpu, thread_state_handle &thread) throw();


    /// A handle on thread state. Implemented differently in the kernel and in
    /// user space.
    struct thread_state_handle {
    private:
        thread_state *state;

    public:

        __attribute__((hot))
        thread_state_handle(void) throw();

        __attribute__((hot, always_inline))
        inline thread_state *operator->(void) throw() {
            return state;
        }
    };


    /// Information maintained by granary about each thread.
    struct thread_state : public client::thread_state {
    private:

        friend struct basic_block;

    };


    /// Define a CPU state handle; in user space, the cpu state is also thread
    /// private.
    struct cpu_state_handle {
    private:
        cpu_state *state;

    public:

        __attribute__((hot))
        cpu_state_handle(void) throw();

        __attribute__((hot, always_inline))
        inline cpu_state *operator->(void) throw() {
            return state;
        }

        IF_KERNEL( static void init(void) throw(); )
    };


    namespace detail {


        /// For small allocations.
        struct small_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = false,
                TRANSIENT = false,
                SHARED = false,
                EXEC_WHERE = EXEC_NONE
            };
        };

        /// CPU-private fragment allocators.
        struct fragment_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE * 16,
                EXECUTABLE = true,
                TRANSIENT = false,
                SHARED = false,
                EXEC_WHERE = EXEC_CODE_CACHE
            };
        };


        struct instruction_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = true,
                TRANSIENT = true,
                SHARED = false,
                EXEC_WHERE = EXEC_GEN_CODE
            };
        };

        struct wrapper_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = true,
                TRANSIENT = false,
                SHARED = true,
                EXEC_WHERE = EXEC_WRAPPER
            };
        };


        /// CPU-private block-local storage allocators.
        struct block_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = false,
                TRANSIENT = false,
                SHARED = false,
                EXEC_WHERE = EXEC_NONE
            };
        };


        /// Shared/gencode fragment allocators.
        struct global_fragment_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = true,
                TRANSIENT = false,
                SHARED = true,
                EXEC_WHERE = EXEC_GEN_CODE
            };
        };


        /// Transient memory allocator.
        struct transient_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE / 2,
                EXECUTABLE = false,
                TRANSIENT = true,
                SHARED = false,
                EXEC_WHERE = EXEC_NONE
            };
        };


        /// Meta information for the cpu-private code cache.
        struct cpu_code_cache_meta {
            enum {
                HASH_SEED = 0xDEADBEEFU,
            };

            enum {
                DEFAULT_SCALE_FACTOR = 8U,
                MAX_SCAN_SCALE_FACTOR = 8U
            };

            __attribute__((hot))
            FORCE_INLINE static uint32_t hash(const app_pc target_) throw() {
                const uint64_t target(reinterpret_cast<uint64_t>(target_));
                uint32_t h(target);

                /// 32-bit finalizer from Murmurhash3 on the 32-low-order bits.
                h ^= h >> 16;
                h *= 0x85ebca6b;
                h ^= h >> 13;
                h *= 0xc2b2ae35;
                h ^= h >> 16;
                return h;
            }
        };
    }


    /// Information maintained by granary about each CPU.
    ///
    /// Note: when in kernel space, we assume that this state
    /// 	  is only accessed with interrupts disabled.
    struct cpu_state : public client::cpu_state {
    public:

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
