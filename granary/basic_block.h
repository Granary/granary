/*
 * bb.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_BB_H_
#define granary_BB_H_

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/detach.h"

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
    typedef enum {
        BB_BYTE_NATIVE         = (1 << 0),
        BB_BYTE_DELAY_BEGIN    = (1 << 1),
        BB_BYTE_DELAY_CONT     = (1 << 2),
        BB_BYTE_DELAY_END      = (1 << 3)
    } code_cache_byte_state;


    /// Defines the meta-information block that ends each basic block in the
    /// code cache.
    struct basic_block_info {
    public:

        enum {
            HEADER        = 0xD4D5D682
        };

        /// magic number (sequence of 4 int3 instructions) which signals the
        /// beginning of a bb_meta block.
        uint32_t magic;

        /// Number of bytes in this basic block, *including* the number of
        /// bytes of padding, and the patch bytes.
        uint16_t num_bytes;

        /// Number of bytes of patch instructions beginning this basic block.
        uint16_t num_patch_bytes;

        /// The application/module to which this basic block belongs
        uint8_t app_id;

        /// Represents the translation policy used to translate this basic
        /// block. This includes policy properties.
        uint8_t policy_bits;

        /// Relative address to this basic block's block-local storage.
        int32_t rel_state_addr;

        uint16_t unused; // TODO: space for later.

        /// The native pc that "generated" the instructions of this basic block.
        /// That is, if we decoded and instrumented some basic block starting at
        /// pc X, then the generating pc is X.
        app_pc generating_pc;

    } __attribute__((packed));


    /// Represents a basic block. Basic blocks are not concrete objects in the
    /// sense that they are used to build basic blocks; they are an abstraction
    /// imposed on some bytes in the code cache as a convenience.
    struct basic_block {
    public:

        friend struct code_cache;

        /// points to the counting set, where every pair of bits represents the
        /// state of some byte in the code cache; this counting set immediately
        /// follows the info block in memory.
        IF_KERNEL(uint8_t *pc_byte_states;)

        /// the meta information for the specific basic block.
        basic_block_info *info;

    public:

        /// the policy of this basic block.
        instrumentation_policy policy;

        /// location information about this basic block
        app_pc cache_pc_start;
        app_pc cache_pc_current;
        app_pc cache_pc_end;

        /// construct a basic block from a pc that points into the code cache.
        basic_block(app_pc current_pc_) throw();

        /// Returns true iff this interrupt must be delayed. If the interrupt
        /// must be delayed then the arguments are updated in place with the
        /// range of code that must be copied and re-relativised in order to
        /// safely execute the interruptible code. The range of addresses is
        /// [begin, end), where the `end` address is the next code cache address
        /// to execute after the interrupt has been handled.
        bool get_interrupt_delay_range(app_pc &, app_pc &) const throw();

        /// Compute the size of a basic block given an instruction list. This
        /// computes the size of each instruction, the amount of padding, meta
        /// information, etc.
        static unsigned size(instruction_list &) throw();

        /// Compute the size of an existing basic block.
        unsigned size(void) const throw();

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

        typedef void (client_instrumenter)(
            cpu_state_handle &cpu,
            thread_state_handle &thread,
            basic_block_state *bb,
            instruction_list &ls);

        /// Decode and translate a single basic block of application/module code.
        ///
        /// TODO: I don't like that I need to pass in an argument to pass out
        ///       for pre-populating the code cache with return addresses back
        ///       into basic blocks.
        static basic_block translate(
            instrumentation_policy policy,
            cpu_state_handle &cpu,
            thread_state_handle &thread,
            app_pc start_pc
        ) throw();


        /// Emit an instruction list as code into a byte array. This will also
        /// emit the basic block meta information and local storage.
        ///
        /// Note: it is assumed that pc is well-aligned, e.g. to an 8 or 16 byte
        ///       boundary.
        ///
        /// Note: it is assumed that enough space has been allocated for the
        ///       instructions and the basic block meta-info, etc.
        ///
        /// Args:
        ///     policy:         The policy of this basic block.
        ///     ls:             The instructions to encode.
        ///     generating_pc:  The program PC whose decoding/translation
        ///                     generated the instruction list ls.
        ///     generated_pc:   A pointer to the memory location where we will
        ///                     store this basic block. When the block is
        ///                     emitted, this pointer is updated to the address
        ///                     of the memory location immediately following
        ///                     the basic block.
        static app_pc emit(
            instrumentation_policy kind,
            instruction_list &ls,
            instruction bb_begin,
            basic_block_state *block_storage,
            app_pc generating_pc,
            app_pc generated_pc
        ) throw();
    };

}

#endif /* granary_BB_H_ */
