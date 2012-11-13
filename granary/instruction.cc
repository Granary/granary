/*
 * instruction.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include <cstring>

#include "granary/instruction.h"

namespace granary {


#define MAKE_REG(name, upper_name) dynamorio::opnd_t name = dynamorio::opnd_create_reg(dynamorio::DR_ ## upper_name);
    namespace reg {
#include "inc/registers.h"
    }
#undef MAKE_REG


    // used frequently in instruction functions
    dynamorio::dcontext_t *instruction::DCONTEXT = \
        dynamorio::get_thread_private_dcontext();


    /// constructor
    instruction::instruction(void) throw() {
        memset(this, 0, sizeof *this);
        dynamorio::instr_set_x86_mode(this, true);
    }

#if 0
    /// Move constructor.
    instruction::instruction(instruction &&that) throw() {
        memcpy(this, &that, sizeof *this);
    }


    /// Move assignment.
    instruction &instruction::operator=(instruction &&that) throw() {
        if(this != &that) {
            memcpy(this, &that, sizeof *this);
        }
        return *this;
    }
#endif

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
        return instr->operator->();
    }


    /// allocate a label for use by this instruction list; NOTE:
    instruction_label
    instruction_list::label(void) throw() {
        instruction lab;
        dynamorio::instr_set_opcode(&lab, dynamorio::OP_LABEL);
        dynamorio::instr_set_num_opnds(nullptr, &lab, 0, 0);
        return instruction_label(allocate(lab));
    }


    /// Insert a label at the end of an instruction list.
    instruction_list::item_type
    instruction_list::append(instruction_label &label) throw() {
        label.is_used = true;
        cache(label.instr);
        return list<instruction>::append(**(label.instr));
    }


    /// Insert a label at the beginning of an instruction list.
    instruction_list::item_type
    instruction_list::prepend(instruction_label &label) throw() {
        label.is_used = true;
        cache(label.instr);
        return list<instruction>::prepend(**(label.instr));
    }


    /// Insert a label before another instruction in an instruction list.
    instruction_list::item_type
    instruction_list::insert_before(item_type pos, instruction_label &label) throw() {
        label.is_used = true;
        cache(label.instr);
        return list<instruction>::insert_before(pos, **(label.instr));
    }


    /// Insert a label after another instruction in an instruction list.
    instruction_list::item_type
    instruction_list::insert_after(item_type pos, instruction_label &label) throw() {
        label.is_used = true;
        cache(label.instr);
        return list<instruction>::insert_after(pos, **(label.instr));
    }
}

