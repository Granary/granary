/*
 * state.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_STATE_H_
#define granary_STATE_H_


#include <bitset>

#include "granary/globals.h"
#include "granary/hash_table.h"

#include "clients/state.h"

namespace granary {


    struct thread_state;
    struct cpu_state;
    struct basic_block_state;
    struct basic_block;


    /// A handle on thread state. Implemented differently in the kernel and in
    /// user space.
    struct thread_state_handle {
    private:
        thread_state *state;

    public:

        thread_state_handle(void) throw();

        inline thread_state *operator->(void) throw() {
            return state;
        }
    };


    /// Information maintained by granary about each thread.
    struct thread_state : public client::thread_state {
    private:

        friend struct basic_block;

    };


    /// Define a CPU state handle; extra layer of indirection for user space
    /// because there is no useful way to know that we're always on the same
    /// cpu.
#if GRANARY_IN_KERNEL
    struct cpu_state_handle {
    private:
        cpu_state *state;

    public:

        cpu_state_handle(void) throw();

        inline cpu_state *operator->(void) throw() {
            return state;
        }
    };
#else
    struct cpu_state_handle {
    private:
        bool has_lock;
    public:
        cpu_state_handle(void) throw();
        cpu_state_handle(cpu_state_handle &&that) throw();
        ~cpu_state_handle(void) throw();
        cpu_state_handle &operator=(cpu_state_handle &&that) throw();
        cpu_state *operator->(void) throw();
    };
#endif


    /// Information maintained by granary about each CPU.
    ///
    /// Note: when in kernel space, we assume that this state
    /// 	  is only accessed with interrupts disabled.
    struct cpu_state : public client::cpu_state {
    private:

        struct fragment_allocator_config {
            enum {
                SLAB_SIZE = 4 * PAGE_SIZE,
                EXECUTABLE = true,
                TRANSIENT = false,
                SHARED = false
            };
        };

        struct transient_allocator_config {
            enum {
                SLAB_SIZE = PAGE_SIZE,
                EXECUTABLE = false,
                TRANSIENT = true,
                SHARED = false
            };
        };

    public:

        bump_pointer_allocator<fragment_allocator_config> fragment_allocator;
        bump_pointer_allocator<transient_allocator_config> transient_allocator;

        bool interrupts_enabled;
    };


    namespace detail {
        struct dummy_block_state : public client::basic_block_state {
            uint64_t __placeholder;
        };
    }


    /// Information maintained within each emitted basic block of
    /// translated application/module code.
    struct basic_block_state : public client::basic_block_state {
    public:

        /// Returns the size of the basic block state, plus any extra
        /// padding needed to follow the basic block state in order for
        /// instructions to be aligned on a 16 byte boundary.
        constexpr inline static unsigned size(void) throw() {
            return (sizeof(detail::dummy_block_state) > sizeof(uint64_t))
                ? (sizeof(basic_block_state) + ALIGN_TO(sizeof(basic_block_state), 16))
                : 0U;
        }
    };
}

#endif /* granary_STATE_H_ */
