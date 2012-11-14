/*
 * bb.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_BB_H_
#define granary_BB_H_

namespace granary {


    enum {
        /// the magic value is a 3 int3 instructions, followed by the number of
        /// instructions in the basic block.
        BB_MAGIC        = 0xCCCCCC00,
        BB_MAGIC_MASK   = 0xFFFFFF00
    };


    /// Forward declaration.
    struct basic_block;


    /// Defines the meta-information block that ends each basic block in the
    /// code cache.
    struct basic_block_info {
    public:

        /// different states of bytes in the code cache.
        enum {
            BYTE_NATIVE = 0,
            BYTE_MANGLED = 1,
            BYTE_INSTRUMENTED = 2,
            BYTE_PADDING = 3
        };

        /// different kinds of basic blocks in the code cache.
        enum {
            KIND_TRANSLATED_FRAGMENT,
            KIND_INTERRUPTED_FRAGMENT,

            KIND_CALL_LOOKUP,
            KIND_JUMP_LOOKUP,

            KIND_ENTER_GRANARY_CALL,
            KIND_EXIT_GRANARY_CALL,

            KIND_ENTER_GRANARY_RETURN,
            KIND_EXIT_GRANARY_RETURN,

            KIND_ENTER_GRANARY_RETURN_INTERRUPT,

            KIND_ENTER_GRANARY,
            KIND_EXIT_GRANARY
        };

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

        basic_block_info *info;

        app_pc start_pc;
        app_pc current_pc;
        app_pc end_pc;

        uint8_t *pc_byte_states;

    public:

        basic_block(app_pc current_pc_) throw();
    };
}

#endif /* granary_BB_H_ */
