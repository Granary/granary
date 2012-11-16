/*
 * instruction.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_INSTRUCTION_H_
#define granary_INSTRUCTION_H_

#include "granary/globals.h"
#include "granary/list.h"

namespace granary {


    /// Forward declarations.
    struct operand;
    struct operand_lea;
    struct instruction;
    struct instruction_label;
    struct instruction_list;


    /// Defines a decoded x86 instruction type. This wraps around DynamoRIO's
    /// Level-3 decoding on x86 instructions.
    struct instruction {
    private:

        template <typename> friend struct list_meta;

        static typename dynamorio::dcontext_t *DCONTEXT;

    public:

        typename dynamorio::instr_t instr;


        /// Constructor
        instruction(void) throw();


        /// Return the number of source operands in this instruction.
        inline unsigned num_sources(void) const throw() {
            return this->instr.num_srcs;
        }


        /// Return the number of destination operands in this instruction.
        inline unsigned num_destinations(void) const throw() {
            return this->instr.num_dsts;
        }


        /// Return true iff this instruction is a cti.
        inline bool is_cti(void) throw() {
            return dynamorio::instr_is_cti(&(this->instr));
        }


        /// Return the original code program counter from the instruction (if
        /// it exists).
        inline app_pc pc(void) const throw() {
            return this->instr.translation;
        }


        /// Return true iff this instruction is mangled.
        inline bool is_mangled(void) const throw() {
            return 0 != (dynamorio::INSTR_HAS_CUSTOM_STUB & this->instr.flags);
        }


        /// Returns the number of bytes needed to represent this instruction when
        /// it is encoded.
        inline unsigned encoded_size(void) throw() {
            return static_cast<unsigned>(
                dynamorio::instr_length(DCONTEXT, &(this->instr)));
        }


        /// Decodes a raw byte, pointed to by *pc, and updated *pc to be the
        /// following byte. The decoded instruction is returned by value. If
        /// the instruction cannot be decoded, then *pc is set to NULL.
        template <typename T>
        static inline instruction decode(T **pc) throw() {
            instruction self;
            uint8_t *byte_pc(unsafe_cast<uint8_t *>(*pc));
            *pc = unsafe_cast<T *>(
                dynamorio::decode_raw(DCONTEXT, byte_pc, &(self.instr)));
            dynamorio::decode(DCONTEXT, byte_pc, &(self.instr));
            dynamorio::instr_set_translation(&(self.instr), byte_pc);
            return self;
        }


        /// encodes an instruction into a sequence of bytes
        template <typename M>
        inline M *encode(M *pc) {

            // address calculation for relative jumps uses the note field
            if(dynamorio::instr_is_label(&(this->instr))
            || dynamorio::instr_is_cti(&(this->instr))) {
                instr.note = pc;
            }

            uint8_t *byte_pc(unsafe_cast<uint8_t *>(pc));
            byte_pc = dynamorio::instr_encode(DCONTEXT, &(this->instr), byte_pc);
            return unsafe_cast<M *>(byte_pc);
        }
    };


    /// Defines an operand generator for LEA operands
    struct operand_lea {
    public:

        typename dynamorio::reg_id_t base;
        typename dynamorio::reg_id_t index;

        int scale;
        int disp;

        /// Add the scale parameter to the LEA operand. Value values are 1, 2,
        /// 4, and 8.
        template <typename T>
        operand_lea operator*(T scale) const throw() {
            static_assert(std::is_integral<T>::value,
                "Scale must have an integral type.");

            operand_lea ret(*this);
            ret.scale = static_cast<int>(scale);
            return ret;
        }

        /// Add in the displacement to the LEA operand.
        template <typename T>
        operand_lea operator+(T disp) const throw() {
            static_assert(std::is_integral<T>::value,
                "Displacement must have an integral type.");

            operand_lea ret(*this);
            ret.disp = static_cast<int>(disp);
            return ret;
        }

        /// Implicit conversion from unpacked to packed LEA operand type.
        inline operator typename dynamorio::opnd_t(void) const throw() {
            return dynamorio::opnd_create_base_disp(
                base, index, scale, disp, dynamorio::OPSZ_lea);
        }
    };


    /// Defines a generic operand
    struct operand : public dynamorio::opnd_t {
    public:

        inline operand(dynamorio::opnd_t &&that) throw() {
            memcpy(this, &that, sizeof *this);
        }

        operand(typename dynamorio::reg_id_t reg_) throw();

        /// De-referencing creates a new operand type
        operand operator*(void) const throw();

        /// Accessing some byte offset from the operand (assuming it points to
        /// some memory)
        operand operator[](int64_t num_bytes) const throw();

        inline operator typename dynamorio::reg_id_t(void) const throw() {
            return value.reg;
        }

        operand_lea operator+(operand index) const throw();

        operand_lea operator+(operand_lea lea) const throw();

        /// Construct an LEA operand with an index and scale. This is to
        /// respect the order of operations. Note: valid values of scale are
        /// 1, 2, 4, and 8.
        template <typename I>
        operand_lea operator*(I scale) const throw() {
            static_assert(std::is_integral<I>::value,
                "Scale must have an integral type.");

            operand_lea ret;
            ret.base = dynamorio::DR_REG_NULL;
            ret.index = value.reg;
            ret.scale = static_cast<int>(scale);
            ret.disp = 0;
            return ret;
        }

        /// Construct an LEA operand with a base and displacement.
        template <typename I>
        operand_lea operator+(I disp) const throw() {
            static_assert(std::is_integral<I>::value,
                "Displacement must have an integral type.");

            operand_lea ret;
            ret.base = dynamorio::DR_REG_NULL;
            ret.index = value.reg;
            ret.scale = 1;
            ret.disp = static_cast<int>(disp);
            return ret;
        }
    };


    /// Declare that instructions form a doubly linked list, where the next/prev
    /// pointers are embedded in the list structure.
    template <>
    struct list_meta<instruction> {
        enum {
            EMBED = false,
            UNROLL = 5,
            NEXT_POINTER_OFFSET = offsetof(dynamorio::instr_t, next),
            PREV_POINTER_OFFSET = offsetof(dynamorio::instr_t, prev)
        };

        static void *allocate(unsigned size) throw() {
            return heap_alloc(nullptr, size);
        }

        static void free(void *val, unsigned size) throw() {
            dynamorio::instr_destroy(
                instruction::DCONTEXT, (dynamorio::instr_t *) val);
            heap_free(nullptr, val, size);
        }
    };


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
    public:

        using list<instruction>::item_type;
        using list<instruction>::handle_type;
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

        /// The encoded size of the instruction list.
        unsigned encoded_size(void) throw();

        /// encodes an instruction list into a sequence of bytes
        template <typename M>
        M *encode(M *pc) {
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
#   include "granary/inc/registers.h"
    }
#undef MAKE_REG
}

#include "granary/gen/instruction.h"

#endif /* granary_INSTRUCTION_H_ */
