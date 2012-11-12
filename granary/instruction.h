/*
 * instruction.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_INSTRUCTION_H_
#define granary_INSTRUCTION_H_

#include <cstdio>

#include <cstring>
#include <stdint.h>

#include "granary/utils.h"
#include "granary/list.h"
#include "granary/heap.h"
#include "granary/types/dynamorio.h"

namespace granary {

    /// Program counter type.
    typedef dynamorio::app_pc app_pc;


    /// Defines a decoded x86 instruction type. This is a straight extension of
    /// DynamoRIO's instruction type.
    struct instruction : public dynamorio::instr_t {
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

        inline app_pc pc(void) throw() {
            return dynamorio::instr_get_app_pc(this);
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
            dynamorio::instr_set_translation(&self, byte_pc);
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


    /// Declare that instructions form a doubly linked list, where the next/prev
    /// pointers are embedded in the list structure.
    template <>
    struct list_meta<instruction> {
        enum {
            EMBED = false,
            UNROLL = 5,
            NEXT_POINTER_OFFSET = offsetof(instruction, next),
            PREV_POINTER_OFFSET = offsetof(instruction, prev)
        };

        static void *allocate(unsigned size) throw() {
            return heap_alloc(nullptr, size);
        }

        static void free(void *val, unsigned size) throw() {
            heap_free(nullptr, val, size);
        }
    };


    /// Represents a list of decoded instructions.
    typedef list<instruction> instruction_list;
}

#endif /* granary_INSTRUCTION_H_ */
