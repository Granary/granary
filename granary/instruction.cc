/*
 * instruction.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/instruction.h"
#include "granary/gen/instruction.h"

namespace granary {


    // used frequently in instruction functions
    typename dynamorio::dcontext_t *instruction::DCONTEXT = \
        dynamorio::get_thread_private_dcontext();


    /// constructor
    instruction::instruction(void) throw() {
        memset(this, 0, sizeof *this);
        dynamorio::instr_set_x86_mode(&(this->instr), false);
    }


    /// Copy assignment.
    instruction &instruction::operator=(const instruction &&that) throw() {
        if(&that == this) {
            return *this;
        }

        // save key info
        dynamorio::byte *bytes(instr.bytes);
        app_pc translation(instr.translation);
        dynamorio::uint eflags(instr.eflags);
        void *note(instr.note);
        dynamorio::instr_t *next(instr.next);
        dynamorio::instr_t *prev(instr.prev);

        memcpy(&instr, &(that.instr), sizeof instr);

        // restore key info
        instr.bytes = bytes;
        instr.translation = translation;
        instr.eflags |= eflags;
        instr.note = note;
        instr.next = next;
        instr.prev = prev;

        return *this;
    }


    /// Encodes an instruction into a sequence of bytes.
    app_pc instruction::encode(app_pc pc) throw() {

        // address calculation for relative jumps uses the note
        // field; second-pass encoding for hot patchable instructions
        // takes advantage of this
        instr.note = pc;

        return dynamorio::instr_encode(DCONTEXT, &instr, pc);
    }


    /// Encodes an instruction into a sequence of bytes, but where the staging
    /// ground is not necessarily the instruction's final location.
    app_pc instruction::stage_encode(app_pc staged_pc, app_pc final_pc) throw() {

        // address calculation for relative jumps uses the note
        // field; second-pass encoding for hot patchable instructions
        // takes advantage of this
        instr.note = final_pc;

        return dynamorio::instr_encode_to_copy(
            DCONTEXT, &instr, staged_pc, final_pc);
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


    /// Used for injecting NOPs before hot-patchable
    typedef decltype(instruction_list().first()) instruction_list_handle;
    static bool mangled_nops(false);
    static instruction nop3(nop3byte_());
    static instruction nop2(nop2byte_());
    static instruction nop1(nop1byte_());


    /// Encodes N bytes of NOPs into an instuction stream.
    static app_pc encode_mangled_nops(app_pc pc, unsigned num_nops) throw() {

        if(!mangled_nops) {
            mangled_nops = true;
            nop3.set_mangled();
            nop2.set_mangled();
            nop1.set_mangled();
        }

        for(; num_nops >= 3; num_nops -= 3) {
            pc = nop3.encode(pc);
        }

        for(; num_nops >= 2; num_nops -= 2) {
            pc = nop2.encode(pc);
        }

        for(; num_nops >= 1; num_nops -= 1) {
            pc = nop1.encode(pc);
        }

        return pc;
    }


    /// encodes an instruction list into a sequence of bytes
    app_pc instruction_list::encode(app_pc pc) throw() {
        if(!length()) {
            return pc;
        }

        handle_type item(first());
        bool has_local_jump(false);

        for(unsigned i = 0, max = length(); i < max; ++i) {

            // x86-64 guaranteed quadword atomic writes so long as
            // the memory location is aligned on an 8-byte boundary;
            // we will assume that we are never patching an instruction
            // longer than 8 bytes
            if(item->is_patchable()) {
                uint64_t forward_align(ALIGN_TO(reinterpret_cast<uint64_t>(pc), 8));
                pc = encode_mangled_nops(pc, forward_align);
            }

            if(item->is_cti()) {
                operand target(item->get_cti_target());
                if(dynamorio::opnd_is_instr(target)) {
                    has_local_jump = true;
                }
            }

            pc = item->encode(pc);
            item = item.next();
        }

        // local jumps within the same basic block might be forward jumps
        // (at least in the case of direct call/jump stubs); re-emit those
        // instructions in place with the now-resolved PCs.
        if(has_local_jump) {
            item = first();
            for(unsigned i = 0, max = length(); i < max; ++i) {
                if(item->is_cti()) {
                    operand target(item->get_cti_target());
                    if(dynamorio::opnd_is_instr(target)) {
                        item->encode(reinterpret_cast<app_pc>(
                            item->instr.note));
                    }
                }
                item = item.next();
            }
        }

        return pc;
    }

    /// Performs a staged encoding of an instruction list into a sequence
    /// of bytes.
    ///
    /// Note: This will not do any fancy jump resolution, alignment, etc.
    app_pc instruction_list::stage_encode(app_pc staged_pc, app_pc final_pc) throw() {
        if(!length()) {
            return staged_pc;
        }

        handle_type item(first());
        for(unsigned i = 0, max = length(); i < max; ++i) {
            app_pc prev_staged_pc(staged_pc);

            staged_pc = item->stage_encode(staged_pc, final_pc);
            final_pc += staged_pc - prev_staged_pc;
            item = item.next();
        }

        return staged_pc;
    }


    /// Decodes a raw byte, pointed to by *pc, and updated *pc to be the
    /// following byte. The decoded instruction is returned by value. If
    /// the instruction cannot be decoded, then *pc is set to NULL.
    instruction instruction::decode(app_pc *pc) throw() {
        instruction self;
        uint8_t *byte_pc(unsafe_cast<uint8_t *>(*pc));
        *pc = dynamorio::decode_raw(DCONTEXT, byte_pc, &(self.instr));
        dynamorio::decode(DCONTEXT, byte_pc, &(self.instr));

        // keep these associations around
        self.instr.translation = byte_pc;
        self.instr.bytes = byte_pc;

        // hack for getting dynamorio to maintain proper rip-relative
        // associations in the encoding process.
        if(dynamorio::instr_is_cti(&(self.instr))) {
            dynamorio::instr_set_raw_bits_valid(&(self.instr), false);

            if(dynamorio::instr_is_cti_short(&(self.instr))) {
                dynamorio::convert_to_near_rel_common(
                    DCONTEXT, nullptr, &(self.instr));
            }
        }

        return self;
    }


    /// The encoded size of the instruction list.
    unsigned instruction_list::encoded_size(void) throw() {
        auto in = first();
        unsigned size(0U);
        for(unsigned i = 0, max = length(); i < max; ++i) {

            if(in->is_patchable()) {
                size += ALIGN_TO(size, 8);
            }

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

