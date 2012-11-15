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
        BB_MAGIC        = 0xCCCCCC00,
        BB_MAGIC_MASK   = 0xFFFFFF00
    };


    /// Forward declarations.
    struct basic_block;
    struct instruction_list;


    /// different states of bytes in the code cache.
    enum code_cache_byte_state {
        BB_BYTE_NATIVE         = (1 << 0),
        BB_BYTE_MANGLED        = (1 << 1),
        BB_BYTE_INSTRUMENTED   = (1 << 2),
        BB_BYTE_PADDING        = (1 << 3)
    };


    /// different kinds of basic blocks in the code cache.
    enum {
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
    };

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
        const uint32_t kind:8;

        /// used to measure some threshold of the "hotness" of this basic block
        uint16_t hotness;

        /// the number of bytes in this basic block, *including* the number of
        /// bytes of padding
        uint16_t num_bytes;

        /// The next *native* pc. For example, if there were a jump instruction,
        /// then this pointer points to the byte immediately following the jump.
        app_pc next_native_pc;

    } __attribute__((packed));


    /// Represents a basic block. Basic blocks are not concrete objects in the
    /// sense that they are used to build basic blocks; they are an abstraction
    /// imposed on some bytes in the code cache as a convenience.
    struct basic_block {
    private:

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
