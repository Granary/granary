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


    /// Defines a decoded x86 instruction type. This is a straight extension of
    /// DynamoRIO's instruction type.
    struct instruction : public dynamorio::instr_t {
    private:

        template <typename> friend struct list_meta;

        static dynamorio::dcontext_t *DCONTEXT;

    public:

        /// Constructor
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

            // address calculation for relative jumps uses the note field
            this->note = pc;

            uint8_t *byte_pc(unsafe_cast<uint8_t *>(pc));
            byte_pc = dynamorio::instr_encode(DCONTEXT, this, byte_pc);
            return unsafe_cast<T *>(byte_pc);
        }
    };


    /// Defines a generic operand
    struct operand : public dynamorio::opnd_t {
    public:

        typedef dynamorio::opnd_t dynamorio_operand;

        operand(dynamorio::opnd_t &&that) throw() {
            memcpy(this, &that, sizeof *this);
        }

        /// De-referencing creates a new operand type
        operand operator*(void) const throw();

        /// Accessing some byte offset from the operand (assuming it points to
        /// some memory)
        operand operator[](int64_t num_bytes) const throw();
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
        typedef list<instruction>::handle_type handle_type;

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

        handle_type append(instruction_label &label) throw();

        handle_type prepend(instruction_label &label) throw();

        handle_type insert_before(handle_type pos, instruction_label &label) throw();

        handle_type insert_after(handle_type pos, instruction_label &label) throw();

        /// encodes an instruction list into a sequence of bytes
        template <typename T>
        T *encode(T *pc) {
            if(!length()) {
                return pc;
            }

            auto item = first();
            for(unsigned i = 0, max = length(); i < max; ++i) {
                pc = item->encode(pc);
                item = item.next();
            }

            return pc;
        }
    };


    /// registers
#define MAKE_REG(name, upper_name) extern operand name;
    namespace reg {
#include "inc/registers.h"
    }
#undef MAKE_REG
}

#endif /* granary_INSTRUCTION_H_ */
