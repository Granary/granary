/*
 * instruction.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_INSTRUCTION_H_
#define granary_INSTRUCTION_H_

#include <utility>

#include "granary/globals.h"
#include "granary/list.h"

namespace granary {


    /// Forward declarations.
    struct operand;
    struct operand_lea;
    struct instruction;
    struct instruction_label;
    struct instruction_list;


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

        inline operand(void) throw() {
            memset(this, 0, sizeof *this);
        }

        inline operand(const dynamorio::opnd_t &&that) throw() {
            memcpy(this, &that, sizeof *this);
        }

        inline operand(const dynamorio::opnd_t &that) throw() {
            memcpy(this, &that, sizeof *this);
        }

        operand(typename dynamorio::reg_id_t reg_) throw();

        inline operand &operator=(dynamorio::opnd_t &&that) throw() {
        	memcpy(this, &that, sizeof *this);
        	return *this;
        }

        /// De-referencing creates a new operand type
        operand operator*(void) const throw();

        /// Accessing some byte offset from the operand (assuming it points to
        /// some memory)
        operand operator[](int64_t num_bytes) const throw();

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

        inline operator uint64_t(void) const throw() {
            return value.immed_int;
        }

        inline operator app_pc(void) const throw() {
            return value.pc;
        }

        inline operator typename dynamorio::reg_id_t(void) const throw() {
            return value.reg;
        }
    };


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


        /// Copy
        instruction(const instruction &) throw() = default;
        instruction &operator=(const instruction &) throw() = default;


        /// Move assignment operator.
        inline instruction(const instruction &&that) throw() {
            memcpy(this, &that, sizeof *this);
        }
        instruction &operator=(const instruction &&that) throw();


        /// Return the opcode of the instruction.
        inline unsigned op_code(void) const throw() {
            return instr.opcode;
        }


        /// Return the number of source operands in this instruction.
        inline unsigned num_sources(void) const throw() {
            return instr.num_srcs;
        }


        /// Return the number of destination operands in this instruction.
        inline unsigned num_destinations(void) const throw() {
            return instr.num_dsts;
        }


        /// Return true iff this instruction is a control-transfer
        /// instruction.
        inline bool is_cti(void) throw() {
            return dynamorio::instr_is_cti(&instr);
        }


        /// Return true iff this instruction is a CALL instruction.
        inline bool is_call(void) throw() {
        	return dynamorio::instr_is_call(&instr);
        }


        /// Return true iff this instruction is a CALL instruction.
		inline bool is_direct_call(void) throw() {
			return dynamorio::instr_is_call_direct(&instr);
		}


		/// Return true iff this instruction is a CALL instruction.
		inline bool is_indirect_call(void) throw() {
			return dynamorio::instr_is_call_indirect(&instr);
		}


		/// If this instruction is a CTI, then return the operand
		/// representing the destination of the CTI.
		inline operand cti_target(void) throw() {
		    return dynamorio::instr_get_target(&instr);
		}


		/// If this instruction is a CTI, then set the target of the instruction.
        inline void set_cti_target(operand target) throw() {
            return dynamorio::instr_set_target(&instr, target);
        }


        /// Return the original code program counter from the instruction (if
        /// it exists).
        inline app_pc pc(void) const throw() {
            return instr.translation;
        }


        /// Set the program counter of the instruction.
        inline void set_pc(app_pc pc) throw() {
            instr.translation = pc;
        }


        /// Return true iff this instruction is mangled.
        inline bool is_mangled(void) const throw() {
            return 0 != (dynamorio::INSTR_HAS_CUSTOM_STUB & instr.flags);
        }


        /// Set the state of the instruction to be mangled.
        inline void set_mangled(void) throw() {
            instr.flags |= dynamorio::INSTR_HAS_CUSTOM_STUB;
        }


        /// Check to see if this instruction can be patched at runtime. If so,
        /// then this instruction needs to be aligned nicely.
        inline bool is_patchable(void) const throw() {
            return 0 != (dynamorio::INSTR_HOT_PATCHABLE & instr.flags);
        }


        /// Set the state of the instruction to be mangled.
        inline void set_patchable(void) throw() {
            instr.flags |= dynamorio::INSTR_HOT_PATCHABLE;
        }


        /// Returns the number of bytes needed to represent this instruction when
        /// it is encoded.
        inline unsigned encoded_size(void) throw() {
            return static_cast<unsigned>(
                dynamorio::instr_length(DCONTEXT, &instr));
        }


        /// Decodes a raw byte, pointed to by *pc, and updated *pc to be the
        /// following byte. The decoded instruction is returned by value. If
        /// the instruction cannot be decoded, then *pc is set to NULL.
        static instruction decode(app_pc *pc) throw();


        /// Encodes an instruction into a sequence of bytes.
        app_pc encode(app_pc pc) throw();


        /// Encodes an instruction into a sequence of bytes, but where the staging
        /// ground is not necessarily the instruction's final location.
        app_pc stage_encode(app_pc staged_pc, app_pc final_pc) throw();


        /// Slightly evil convenience method for implicitly converting instructions
        /// to pointers to their underlying DR type.
        inline operator typename dynamorio::instr_t *(void) throw() {
        	return &instr;
        }


        /// Get easy access to the internal dynamorio instruction structure
        inline dynamorio::instr_t *operator->(void) throw() {
            return &instr;
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
        app_pc encode(app_pc pc) throw();

        /// Performs a staged encoding of an instruction list into a sequence
        /// of bytes.
        ///
        /// Note: This will not do any fancy jump resolution, alignment, etc.
        app_pc stage_encode(app_pc staged_pc, app_pc final_pc) throw();
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
