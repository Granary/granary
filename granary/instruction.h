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
        inline instruction decode(uint8_t **pc) throw() {
            instruction self;
            *pc = dynamorio::decode(DCONTEXT, *pc, &self);
            return self;
        }
    };

}

#endif /* granary_INSTRUCTION_H_ */
