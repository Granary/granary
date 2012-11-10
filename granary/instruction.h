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
#include "granary/types/dynamorio.h"

namespace granary {

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

}

#endif /* granary_INSTRUCTION_H_ */
