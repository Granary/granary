/*
 * instruction.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_INSTRUCTION_H_
#define granary_INSTRUCTION_H_

#include <cstring>
#include <stdint.h>

#include "granary/utils.h"
#include "granary/list.h"
#include "granary/types/dynamorio.h"

namespace granary {

    /// forward declaration
    struct allocator;

    /// Defines a decoded x86 instruction type.
    struct instruction : protected dynamorio::instr_t {
    private:

        static dynamorio::dcontext_t *DCONTEXT;

    public:

        /// constructor
        instruction(void) throw();

        /// return the number of source operands in this instruction
        inline unsigned num_sources(void) const throw() {
            return this->num_srcs;
        }

        /// return the number of destination operands in this instruction
        inline unsigned num_destinations(void) const throw() {
            return this->num_dsts;
        }

        /// return true iff this instruction is a cti
        inline bool is_cti(void) throw() {
            return dynamorio::instr_is_cti(this);
        }

        /// decodes a raw byte, pointed to by *pc, and updated *pc to be the
        /// following byte. The decoded instruction is returned by value. If
        /// the instruction cannot be decoded, then *pc is set to NULL.
        template <typename T>
        static inline instruction decode(T **pc) throw() {
            instruction self;
            uint8_t *byte_pc(unsafe_cast<uint8_t *>(*pc));
            *pc = unsafe_cast<T *>(
                dynamorio::decode_raw(DCONTEXT, byte_pc, &self));
            dynamorio::decode(DCONTEXT, byte_pc, &self);
            return self;
        }

        /// encodes an instruction into a sequence of bytes
        template <typename T>
        inline T *encode(T *pc) {
            uint8_t *byte_pc(unsafe_cast<uint8_t *>(pc));
            byte_pc = dynamorio::instr_encode(DCONTEXT, this, byte_pc);
            return unsafe_cast<T *>(byte_pc);
        }
    };

    // declare that instructions form a doubly linked embedded list.
    template <>
    struct list_meta<instruction> {
        enum {
            EMBED = true,
            UNROLL = 5,
            NEXT_POINTER = true,
            PREV_POINTER = true,
            NEXT_POINTER_OFFSET = offsetof(dynamorio::instr_t, next),
            PREV_POINTER_OFFSET = offsetof(dynamorio::instr_t, prev)
        };
    };

    typedef list<instruction> instruction_list;

    /*
    /// Defines a list of decoded x86 instructions.
    struct instruction_list {
    private:

        unsigned num_instructions;
        instruction *first;
        allocator *allocator;

    public:

        instruction_list(allocator &) throw();

        void append(instruction in) throw();
        void insert_before(instruction &pos, instruction in) throw();

        void prepend(instruction in) throw();
        void insert_after(instruction &pos, instruction in) throw();
    };
    */
}

#endif /* granary_INSTRUCTION_H_ */
