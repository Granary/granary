/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bb.h
 *
 *      Author: Peter Goodman
 */

#ifndef granary_BB_H_
#define granary_BB_H_

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/detach.h"
#include "granary/state.h"

namespace granary {

    /// Forward declarations.
    struct basic_block;
    struct basic_block_state;
    struct instruction_list;
    struct instruction;
    struct cpu_state_handle;
    struct thread_state_handle;
    struct instrumentation_policy;
    struct code_cache;


    /// different states of bytes in the code cache.
    enum code_cache_byte_state {
        BB_BYTE_NATIVE         = (1 << 0),
        BB_BYTE_DELAY_BEGIN    = (1 << 1),
        BB_BYTE_DELAY_CONT     = (1 << 2),
        BB_BYTE_DELAY_END      = (1 << 3)
    };


    /// Defines the meta-information block that ends each basic block in the
    /// code cache._InputIterator
    struct basic_block_info {
    public:

        /// The starting pc of the basic block in the code cache.
        app_pc start_pc;

        /// The native pc that "generated" the instructions of this basic block.
        /// That is, if we decoded and instrumented some basic block starting at
        /// pc X, then the generating pc is X.
        mangled_address generating_pc;

        //-------------------

        /// Number of bytes in this basic block.
        uint16_t num_bytes;

        /// Number of instructions in the generating basic block.
        uint16_t generating_num_instructions;

#if CONFIG_ENABLE_INTERRUPT_DELAY
        uint16_t num_delay_state_bytes;
        uint16_t _;
#else
        uint32_t _;
#endif

        //-------------------

        /// Address of client/tool-created basic block meta-data.
        basic_block_state *state;

#if CONFIG_ENABLE_INTERRUPT_DELAY
        /// State-set of delay range information for this basic block, if any.
        uint8_t *delay_states;
#endif

        /// Does this basic block look like it might have a user space access
        /// in it? If so, then this is an offset from the address of this field
        /// to a pointer to a kernel exception table entry.
        IF_KERNEL( void *user_exception_metadata; )

#if CONFIG_ENABLE_TRACE_ALLOCATOR
        /// What was the allocator used to create this basic block?
        generic_fragment_allocator *allocator;
#endif

    } __attribute__((packed));


    /// Represents a basic block. Basic blocks are not concrete objects in the
    /// sense that they are used to build basic blocks; they are an abstraction
    /// imposed on some bytes in the code cache as a convenience.
    struct basic_block {
    public:

        friend struct code_cache;


        /// The meta information for the specific basic block.
        basic_block_info *info;


    public:


        /// The instrumentation policy of used to translate the code for this
        /// basic block.
        instrumentation_policy policy;


        /// Beginning on native/instrumented instructions within the basic
        /// block. This skips past any emitted stub instructions at the
        /// beginning of the basic block.
        app_pc cache_pc_start;


        /// The current instruction within the basic block. This is the
        /// instruction that was used to "find" and "rebuild" this information
        /// about the basic block from the code cache.
        app_pc cache_pc_current;


        /// construct a basic block from a pc that points into the code cache.
        basic_block(app_pc current_pc_) throw();


#if GRANARY_IN_KERNEL && CONFIG_ENABLE_INTERRUPT_DELAY
        /// Returns true iff this interrupt must be delayed. If the interrupt
        /// must be delayed then the arguments are updated in place with the
        /// range of code that must be copied and re-relativised in order to
        /// safely execute the interruptible code. The range of addresses is
        /// [begin, end), where the `end` address is the next code cache address
        /// to execute after the interrupt has been handled.
        bool get_interrupt_delay_range(app_pc &, app_pc &) const throw();
#endif


        /// Call the code within the basic block as if is a function.
        template <typename R, typename... Args>
        R call(Args... args) throw() {
            typedef R (func_type)(Args...);
            function_call<R, Args...> func(
                unsafe_cast<func_type *>(cache_pc_start));
            func(args...);
            detach();
            return func.yield();
        }


    protected:


        /// Decode instructions at a starting program counter and end decoding
        /// when we've met the ending conditions for a basic block.
        static unsigned decode(
            instruction_list &ls,
            instrumentation_policy policy,
            const app_pc start_pc
            _IF_KERNEL( void *&user_exception_metadata )
        ) throw();


        /// Decode and translate a single basic block of application/module code.
        static basic_block translate(
            const instrumentation_policy policy,
            cpu_state_handle cpu,
            const app_pc start_pc
        ) throw();


    public:

        /// Return a pointer to the basic block state structure of this basic
        /// block.
        inline basic_block_state *state(void) const throw() {
            return info->state;
        }
    };

}

#endif /* granary_BB_H_ */
