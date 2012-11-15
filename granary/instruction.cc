/*
 * instruction.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/instruction.h"

namespace granary {


    // used frequently in instruction functions
    typename dynamorio::dcontext_t *instruction::DCONTEXT = \
        dynamorio::get_thread_private_dcontext();


    /// constructor
    instruction::instruction(void) throw() {
        memset(this, 0, sizeof *this);
        dynamorio::instr_set_x86_mode(&(this->instr), true);
    }


    /// Implicit constructor for converting operand_lea's to operands.
    operand::operand(operand_lea &&that) throw() {
        operand op_lea(mem_lea_(that.base, that.index, that.scale, that.disp));
        memcpy(this, &op_lea, sizeof *this);
    }


    /// Implicit constructor for registers.
    operand::operand(typename dynamorio::reg_id_t reg_) throw() {
        *this = dynamorio::opnd_create_reg(reg_);
    }

    /// De-reference an address stored in a register
    operand operand::operator*(void) const throw() {
        return mem64_(value.reg, 0);
    }


    /// Accessing some byte offset from the operand (assuming it points to
    /// some memory).
    operand operand::operator[](int64_t num_bytes) const throw() {
        return mem64_(value.reg, num_bytes);
    }


    /// Add an index into an lea operand
    operand_lea operand::operator+(operand index) const throw() {
        operand_lea ret;
        ret.base = value.reg;
        ret.index = index.value.reg;
        ret.scale = 1;
        ret.disp = 0;
        return ret;
    }


    /// Add in the base register to this LEA operand.
    operand_lea operand::operator+(operand_lea lea) const throw() {
        lea.base = value.reg;
        return lea;
    }


    /// Private instruction label constructor. Stores the raw pointer to the
    /// list item. This is a leaky abstraction :-P
    instruction_label::instruction_label(item_type *instr_) throw()
        : instr(instr_)
        , is_used(false)
    { }


    /// Move constructor.
    instruction_label::instruction_label(instruction_label &&that) throw()
        : instr(that.instr)
        , is_used(that.is_used)
    { }


    /// Destroy the instruction label; the item associated with the label is
    /// destroyed iff the label was not used in any list.
    instruction_label::~instruction_label(void) throw() {
        if(!is_used && instr) {
            list_meta<instruction>::free(instr, sizeof *instr);
        }
        instr = nullptr;
    }


    /// Move assignment operator.
    instruction_label &
    instruction_label::operator=(instruction_label &&that) throw() {
        if(this != &that) {
            instr = that.instr;
            is_used = that.is_used;

            that.instr = nullptr;
            that.is_used = false;
        }
        return *this;
    }


    /// Get a pointer to the internal dynamorio::instr_t for use by
    /// control-flow instructions.
    instruction_label::operator instruction *(void) throw() {
        return &(**instr);
    }


    /// allocate a label for use by this instruction list; NOTE:
    instruction_label
    instruction_list::label(void) throw() {
        item_type *item = allocate(label_());
        return instruction_label(item);
    }


    /// Insert a label at the end of an instruction list.
    instruction_list::handle_type
    instruction_list::append(instruction_label &label) throw() {
        label.is_used = true;
        return this->list<instruction>::append(label.instr);
    }


    /// Insert a label at the beginning of an instruction list.
    instruction_list::handle_type
    instruction_list::prepend(instruction_label &label) throw() {
        label.is_used = true;
        return this->list<instruction>::prepend(label.instr);
    }


    /// Insert a label before another instruction in an instruction list.
    instruction_list::handle_type
    instruction_list::insert_before(handle_type pos, instruction_label &label) throw() {
        label.is_used = true;
        return this->list<instruction>::insert_before(get_item(pos), label.instr);
    }


    /// Insert a label after another instruction in an instruction list.
    instruction_list::handle_type
    instruction_list::insert_after(handle_type pos, instruction_label &label) throw() {
        label.is_used = true;
        return this->list<instruction>::insert_after(get_item(pos), label.instr);
    }


    /// The encoded size of the instruction list.
    unsigned instruction_list::encoded_size(void) throw() {
        auto in = first();
        unsigned size(0U);
        for(unsigned i = 0, max = length(); i < max; ++i) {
            size += in->encoded_size();
            in = in.next();
        }
        return size;
    }

#define MAKE_REG(name, upper_name) operand name = \
    dynamorio::opnd_create_reg(dynamorio::DR_ ## upper_name);

    namespace reg {
#   include "granary/inc/registers.h"
    }
#undef MAKE_REG

}

