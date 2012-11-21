/*
 * bb.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_BB_H_
#define granary_BB_H_

#include "granary/globals.h"

namespace granary {

    /// Forward declarations.
    struct basic_block;
    struct basic_block_state;
    struct instruction_list;
    struct cpu_state_handle;


    /// different states of bytes in the code cache.
    typedef enum {
        BB_BYTE_NATIVE         = (1 << 0),
        BB_BYTE_MANGLED        = (1 << 1),
        BB_BYTE_INSTRUMENTED   = (1 << 2),
        BB_BYTE_PADDING        = (1 << 3)
    } code_cache_byte_state;


    /// different kinds of basic blocks in the code cache.
    typedef enum {
        BB_TRANSLATED_FRAGMENT,
        BB_INTERRUPTED_FRAGMENT,

        BB_CALL_LOOKUP,
        BB_JUMP_LOOKUP,

        BB_ENTER_GRANARY_CALL,
        BB_EXIT_GRANARY_CALL,

        BB_ENTER_GRANARY_RETURN,
        BB_EXIT_GRANARY_RETURN,

        BB_ENTER_GRANARY_RETURN_INTERRUPT,

        BB_ENTER_GRANARY,
        BB_EXIT_GRANARY
    } basic_block_kind;


    /// Defines the meta-information block that ends each basic block in the
    /// code cache.
    struct basic_block_info {
    public:


        /// magic number (sequence of 4 int3 instructions) which signals the
        /// beginning of a bb_meta block.
        uint32_t magic:24;

        /// the kind of this basic block.
        basic_block_kind kind:8;

        /// the number of bytes in this basic block, *including* the number of
        /// bytes of padding
        uint16_t num_bytes;

        /// The application/module to which this basic block belongs
        uint8_t app_id;

        /// used to measure some threshold of the "hotness" of this basic block
        volatile uint8_t hotness;

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

        /// points to the counting set, where every pair of bits represents the
        /// state of some byte in the code cache; this counting set immediately
        /// follows the info block in memory.
        uint8_t *pc_byte_states;

        /// the meta information for the specific basic block.
        basic_block_info *info;

    public:

        /// The block-local storage for this block
        basic_block_state *state;

        /// location information about this basic block
        app_pc cache_pc_start;
        app_pc cache_pc_current;
        app_pc cache_pc_end;

        /// construct a basic block from a pc that points into the code cache.
        basic_block(app_pc current_pc_) throw();

        /// Returns the next safe interrupt location.
        app_pc next_safe_interrupt_location(void) const throw();

        /// Compute the size of a basic block given an instruction list. This
        /// computes the size of each instruction, the amount of padding, meta
        /// information, etc.
        static unsigned size(instruction_list &) throw();

        /// Compute the size of an existing basic block.
        unsigned size(void) const throw();

        /// Decode and translate a single basic block of application/module code.
        static basic_block translate(cpu_state_handle &cpu, app_pc *pc) throw();

    //protected:

        /// Emit an instruction list as code into a byte array. This will also
        /// emit the basic block meta information and local storage.
        ///
        /// Note: it is assumed that pc is well-aligned, e.g. to an 8 or 16 byte
        ///       boundary.
        ///
        /// Args:
        ///     kind:           The kind of this basic block.
        ///     ls:             The instructions to encode.
        ///     generating_pc:  The program PC whose decoding/translation
        ///                     generated the instruction list ls.
        ///     generated_pc:   A pointer to the memory location where we will
        ///                     store this basic block. When the block is
        ///                     emitted, this pointer is updated to the address
        ///                     of the memory location immediately following
        ///                     the basic block.
        static basic_block emit(basic_block_kind kind,
        						instruction_list &ls,
        						app_pc generating_pc,
                                app_pc *generated_pc) throw();
    };
}

#endif /* granary_BB_H_ */
