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

    enum {
        /// the magic value is a 3 int3 instructions, followed by the number of
        /// instructions in the basic block.
        BB_PADDING      = 0xCC,
        BB_MAGIC        = 0xCCCCCC00,
        BB_MAGIC_MASK   = 0xFFFFFF00
    };


    /// Forward declarations.
    struct basic_block;
    struct instruction_list;


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


    enum {
        BB_BYTE_STATES_PER_BYTE = 4
    };


    /// Defines the meta-information block that ends each basic block in the
    /// code cache.
    struct basic_block_info {
    public:


        /// magic number (sequence of 4 int3 instructions) which signals the
        /// beginning of a bb_meta block.
        const uint32_t magic:24;

        /// the kind of this basic block.
        const basic_block_kind kind:8;

        /// used to measure some threshold of the "hotness" of this basic block
        volatile uint16_t hotness;

        /// the number of bytes in this basic block, *including* the number of
        /// bytes of padding
        uint16_t num_bytes;

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

        /// Emit an instruction list as code into a byte array. This will also
        /// emit the basic block meta information.
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
        static basic_block emit(basic_block_kind kind, instruction_list &ls,
                                app_pc generating_pc, app_pc *generated_pc) throw();

    private:

        /// return the state of a specific byte in the code cache
        inline code_cache_byte_state state_of(app_pc addr) const throw() {
            const unsigned i((addr - cache_pc_start));
            const unsigned j(i / BB_BYTE_STATES_PER_BYTE); // byte offset
            const unsigned k(2 * (i % BB_BYTE_STATES_PER_BYTE)); // bit offset
            return static_cast<code_cache_byte_state>(
                1 << ((pc_byte_states[j] >> k) & 0x03));
        }
    };
}

#endif /* granary_BB_H_ */
