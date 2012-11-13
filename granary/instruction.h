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
#include "granary/heap.h"
#include "granary/types/dynamorio.h"

namespace granary {

    /// Program counter type.
    typedef dynamorio::app_pc app_pc;

    /// registers
#define MAKE_REG(name, upper_name) extern dynamorio::opnd_t name;
    namespace reg {
#include "inc/registers.h"
    }
#undef MAKE_REG

    /// Defines a decoded x86 instruction type. This is a straight extension of
    /// DynamoRIO's instruction type.
    struct instruction : public dynamorio::instr_t {
    private:

        template <typename> friend struct list_meta;

        static dynamorio::dcontext_t *DCONTEXT;

    public:

        /// Constructor
        instruction(void) throw();

#if 0
        /// Move constructor.
        instruction(instruction &&) throw();

        /// Move assignment.
        instruction &operator=(instruction &&) throw();
#endif

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
            dynamorio::instr_destroy(instruction::DCONTEXT, (dynamorio::instr_t *) val);
            heap_free(nullptr, val, size);
        }
    };


    /// Forward declaration.
    struct instruction_list;


    /// Represents an instruction label.
    struct instruction_label {
    private:

        friend struct instruction_list;
        typedef list<instruction>::item_type item_type;

        item_type *instr;
        bool is_used;

        instruction_label(item_type *instr_) throw();

    public:

        instruction_label(instruction_label &&that) throw();
        ~instruction_label(void) throw();

        instruction_label &operator=(instruction_label &&that) throw();

        /// Get a pointer to the internal dynamorio::instr_t for use by
        /// control-flow instructions.
        operator instruction *(void) throw();
    };


    /// Represents a list of Level 3 instructions.
    struct instruction_list : public list<instruction> {
    private:

        typedef list<instruction>::item_type item_type;

    public:

        using list<instruction>::append;
        using list<instruction>::prepend;
        using list<instruction>::insert_before;
        using list<instruction>::insert_after;

        /// allocate a label for use by this instruction list; NOTE:
        instruction_label label(void) throw();

        item_type append(instruction_label &label) throw();

        item_type prepend(instruction_label &label) throw();

        item_type insert_before(item_type pos, instruction_label &label) throw();

        item_type insert_after(item_type pos, instruction_label &label) throw();
    };
}

#endif /* granary_INSTRUCTION_H_ */
