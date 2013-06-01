/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instruction.cc
 *
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/instruction.h"
#include "granary/gen/instruction.h"

namespace granary {


    /// Operand that represents a PC stored somewhere in memory.
    operand mem_pc_(app_pc *pc) {
        operand op;
        op.kind = dynamorio::BASE_DISP_kind;
        op.value.addr = pc;
        op.size = dynamorio::OPSZ_8;
        op.seg.segment = dynamorio::DR_REG_NULL;
        return op;
    }


    /// used frequently in instruction functions
    typename dynamorio::dcontext_t *instruction::DCONTEXT = nullptr;


    STATIC_INITIALISE_ID(dcontext, {
        instruction::DCONTEXT = dynamorio::get_thread_private_dcontext();
    })


    instruction jecxz_(dynamorio::opnd_t t) {
        return dynamorio::instr_create_0dst_2src(
            instruction::DCONTEXT,
            dynamorio::OP_jecxz,
            (t),
            dynamorio::opnd_create_reg(dynamorio::DR_REG_ECX)
        );
    }


    static dynamorio::instr_t *make_instr(void) throw() {
        auto instr = (dynamorio::instr_t *) granary_heap_alloc(
            nullptr, sizeof (dynamorio::instr_t));
        dynamorio::instr_set_x86_mode(instr, false);
        instr->flags |= dynamorio::INSTR_HAS_CUSTOM_STUB;
        return instr;
    }


    /// Copy assignment.
    void instruction::replace_with(instruction that) throw() {

        if(that.instr == instr) {
            return;
        }

        // save key info
        //dynamorio::byte *bytes(instr->bytes);
        //dynamorio::uint eflags(instr->eflags);

        app_pc translation(instr->translation);
        void *note(instr->note);
        dynamorio::instr_t *next_(instr->next);
        dynamorio::instr_t *prev_(instr->prev);
        unsigned granary_flags(instr->granary_flags);
        unsigned granary_policy(instr->granary_flags);

        memcpy(instr, that.instr, sizeof *instr);

        // TODO:
        // will need to decide what the best approach is here; there are
        // interesting interactions when dealing with nop2byte_, which manually
        // allocates is bits, and when such a nop is (stage)encoded and then
        // assigned to another instruction.

        // restore key info
        //instr->bytes = bytes;
        //instr->eflags = eflags;
        instr->translation = translation;
        instr->note = note;
        instr->next = next_;
        instr->prev = prev_;
        instr->granary_flags = granary_flags;
        instr->granary_policy = granary_policy;
    }


    /// Encodes an instruction into a sequence of bytes.
    app_pc instruction::encode(app_pc pc_) throw() {

        // address calculation for relative jumps uses the note
        // field; second-pass encoding for hot patchable instructions
        // takes advantage of this
        instr->note = pc_;
        app_pc ret(dynamorio::instr_encode(DCONTEXT, instr, pc_));

        instr->translation = pc_;
        return ret;
    }


    /// Encodes an instruction into a sequence of bytes, but where the staging
    /// ground is not necessarily the instruction's final location.
    app_pc instruction::stage_encode(app_pc staged_pc, app_pc final_pc) throw() {

        // address calculation for relative jumps uses the note
        // field; second-pass encoding for hot patchable instructions
        // takes advantage of this
        instr->note = final_pc;

        return dynamorio::instr_encode_to_copy(
            DCONTEXT, instr, staged_pc, final_pc);
    }


    /// Implicit constructor for registers.
    operand::operand(typename dynamorio::reg_id_t reg_) throw() {
        *this = dynamorio::opnd_create_reg(reg_);
    }

    /// De-reference an address stored in a register
    operand_base_disp operand::operator*(void) const throw() {
        operand_base_disp op;
        op.base = value.reg;
        op.size = size;
        return op;
    }


    /// Accessing some byte offset from the operand (assuming it points to
    /// some memory).
    operand_base_disp operand::operator[](int64_t num_bytes) const throw() {
        operand_base_disp op;
        op.base = value.reg;
        op.disp = num_bytes;
        op.size = size;
        return op;
    }


    operand_base_disp::operand_base_disp(void) throw()
        : base(dynamorio::DR_REG_NULL)
        , index(dynamorio::DR_REG_NULL)
        , size(dynamorio::OPSZ_lea)
        , scale(1)
        , disp(0)
    { }


    /// Add an index into an lea operand
    operand_base_disp operand::operator+(operand index) const throw() {
        operand_base_disp ret;
        ret.base = value.reg;
        ret.index = index.value.reg;
        ret.scale = 1;
        ret.disp = 0;
        return ret;
    }


    /// Add in the base register to this LEA operand.
    operand_base_disp operand::operator+(operand_base_disp lea) const throw() {
        lea.base = value.reg;
        return lea;
    }


    /// Initialise the operand ref with an instruction and operand pointer
    operand_ref::operand_ref(
        dynamorio::instr_t *instr_,
        dynamorio::opnd_t *op_,
        operand_kind kind_
    ) throw()
        : instr(instr_)
        , op(unsafe_cast<operand *>(op_))
        , op2(nullptr)
        , kind(kind_)
    { }


    /// Assume that a non-const access of a field of the op will be used
    /// as an lvalue in an assignment; invalidate the raw bits.
    operand *operand_ref::operator->(void) throw() {
        ASSERT(!op2);
        instruction(instr).invalidate_raw_bits();
        return op;
    }


    /// Assign an operand to this operand ref; this will update the operand
    /// referenced by this ref in place, and will invalidate the raw bits
    /// of the instruction.
    operand_ref &operand_ref::operator=(operand that) throw() {
        *op = that;
        if(op2) {
            *op2 = that;
        }
        instruction(instr).invalidate_raw_bits();
        return *this;
    }

    operand_ref &operand_ref::operator=(operand_base_disp that) throw() {
        *op = operand(that);
        if(op2) {
            *op2 = *op;
        }
        instruction(instr).invalidate_raw_bits();
        return *this;
    }


    /// Augment this `operand_ref` to a `SOURCE_DEST_OPERAND` if possible
    /// by combining this operand with another.
    void operand_ref::combine(const operand_ref &that) const throw() {
        ASSERT(can_combine(that));
        kind = SOURCE_DEST_OPERAND;
        op2 = that.op;
    }


    /// Returns true iff two `operand_refs` can be combined.
    bool operand_ref::can_combine(const operand_ref &that) const throw() {
        return instr == that.instr
            && op != that.op
            && op2 == that.op2
            && nullptr == op2
            && kind != that.kind
            && nullptr != op
            && 0 == memcmp(op, that.op, sizeof *op);
    }


    /// Move constructor.
    instruction_list::instruction_list(self_type &&that) throw()
        : first_(that.first_)
        , last_(that.last_)
        , length_(that.length_)
    {
        that.first_ = nullptr;
        that.last_ = nullptr;
        that.length_ = 0U;
    }


    /// Clear the elements of the list, and release any memory associated
    /// with the elements of the list
    void instruction_list::clear(void) throw() {
        if(!first_) {
            return;
        }

        first_ = nullptr;
        last_ = nullptr;
        length_ = 0;
    }


    /// Adds an element on to the end of the list.
    instruction instruction_list::append(instruction item_) throw() {

        dynamorio::instr_t *item(item_.instr);
        dynamorio::instr_t *before_item(last_);

        last_ = item;
        item->prev = before_item;
        item->next = nullptr;
        if(before_item) {
            before_item->next = item;
        }

        if(1 == ++length_) {
            first_ = item;
        }

        return item_;
    }

    /// Adds an element on to the beginning of the list.
    instruction instruction_list::prepend(instruction item_) throw() {
        dynamorio::instr_t *item(item_.instr);
        dynamorio::instr_t *after_item(first_);

        first_ = item;
        item->next = after_item;
        item->prev = nullptr;

        if(after_item) {
            after_item->prev = item;
        }

        if(1 == ++length_) {
            last_ = item;
        }

        return item_;
    }

    /// Insert an element before another object in the list.
    instruction instruction_list::insert_before(
        instruction after_item_,
        instruction item_
    ) throw() {

        if(1 >= length_ || !after_item_) {
            return prepend(item_);
        }

        dynamorio::instr_t *after_item(after_item_.instr);
        dynamorio::instr_t *item(item_.instr);
        dynamorio::instr_t *before_item(after_item->prev);

        if(!before_item) {
            return prepend(item);
        }

        return chain(before_item, item, after_item);
    }


    /// Insert an element after another object in the list
    instruction instruction_list::insert_after(
        instruction before_item_,
        instruction item_
    ) throw() {

        if(1 >= length_ || !before_item_) {
            return append(item_);
        }

        dynamorio::instr_t *before_item(before_item_.instr);
        dynamorio::instr_t *item(item_.instr);
        dynamorio::instr_t *after_item(before_item->next);

        if(!after_item) {
            return append(item);
        }

        return chain(before_item, item, after_item);
    }


    /// Remove an element from an instruction list.
    void instruction_list::remove(instruction to_remove) throw() {
        dynamorio::instr_t *instr(to_remove.instr);
        if(!instr) {
            return;
        }

        ASSERT(0 < length_);

        dynamorio::instr_t *prev(instr->prev);
        dynamorio::instr_t *next(instr->next);

        if(prev) {
            prev->next = next;
            instr->prev = nullptr;
        } else {
            first_ = next;
        }

        if(next) {
            next->prev = prev;
            instr->next = nullptr;
        } else {
            last_ = prev;
        }

        --length_;
    }


    /// Chain an element into the list.
    instruction instruction_list::chain(
        dynamorio::instr_t *before_item,
        dynamorio::instr_t *item,
        dynamorio::instr_t *after_item
    ) throw() {
        ++length_;

        item->prev = before_item;
        before_item->next = item;

        item->next = after_item;
        after_item->prev = item;

        return instruction(item);
    }


    /// encodes an instruction list into a sequence of bytes
    app_pc instruction_list::encode(app_pc start_pc) throw() {
        if(!length()) {
            return start_pc;
        }

        instruction item(first());
        app_pc pc(start_pc);
        bool has_local_jump(false);

        for(unsigned i = 0, max = length(); i < max; ++i) {

            dynamorio::instr_t *target_instr(nullptr);

            if(item.is_cti()) {
                operand target(item.cti_target());

                // temporarily point to itself.
                if(dynamorio::opnd_is_instr(target)) {
                    target_instr = target.value.instr;
                    item.instr->u.o.src0.value.instr = item.instr;
                    has_local_jump = true;
                }
            }

            pc = item.encode(pc);

            // restore its correct target for later jump resolution.
            if(target_instr) {
                item.instr->u.o.src0.value.instr = target_instr;
            }

            IF_PERF( perf::visit_encoded(item); )

            item = item.next();
        }

        // local jumps within the same basic block might be forward jumps
        // (at least in the case of direct call/jump stubs); re-emit those
        // instructions in place with the now-resolved PCs.
        if(has_local_jump) {
            item = first();
            for(unsigned i = 0, max = length(); i < max; ++i) {
                if(item.is_cti()) {
                    operand target(item.cti_target());
                    if(dynamorio::opnd_is_instr(target)) {
                        item.encode(reinterpret_cast<app_pc>(item.instr->note));
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

        instruction in(first());
        for(; in.is_valid(); in = in.next()) {
            app_pc prev_staged_pc(staged_pc);
            staged_pc = in.stage_encode(staged_pc, final_pc);
            final_pc += staged_pc - prev_staged_pc;
        }

        return staged_pc;
    }


    /// Widen this instruction if its a CTI.
    void instruction::widen_if_cti(void) throw() {

        // hack for getting dynamorio to maintain proper rip-relative
        // associations in the encoding process.
        if(dynamorio::instr_is_cti(instr)) {
            dynamorio::instr_set_raw_bits_valid(instr, false);

            if(dynamorio::instr_is_cti_short(instr)) {
                dynamorio::convert_to_near_rel_common(
                    DCONTEXT, nullptr, instr);
            }
        }
    }


#if !GRANARY_IN_KERNEL
    enum {
        X86_INT3 = 0xCC,
        X86_REPZ = 0xF3,
        X86_RET_SHORT = 0xC3
    };
#endif


    /// Decodes a raw byte, pointed to by *pc, and updated *pc to be the
    /// following byte. The decoded instruction is returned by value. If
    /// the instruction cannot be decoded, then *pc is set to NULL.
    instruction instruction::decode(app_pc *pc) throw() {
        instruction self(make_instr());
        uint8_t *byte_pc(*pc);

#if !GRANARY_IN_KERNEL
        // Deal with `REPZ RET` in user space with potential GDB breakpoints.
        if((X86_INT3 == byte_pc[0] || X86_REPZ == byte_pc[0])
        && X86_RET_SHORT == byte_pc[1]) {
            ++*pc;
            ++byte_pc;
        }
#endif

        *pc = dynamorio::decode_raw(DCONTEXT, byte_pc, self.instr);
        dynamorio::decode(DCONTEXT, byte_pc, self.instr);

        // keep these associations around
        self.instr->translation = byte_pc;
        self.instr->bytes = byte_pc;

        IF_PERF( perf::visit_decoded(self); )

        self.widen_if_cti();

        return self;
    }


    /// The encoded size of the instruction list.
    unsigned instruction_list::encoded_size(void) throw() {
        auto in = first();
        unsigned size(0U);
        for(; in.is_valid(); in = in.next()) {
            size += in.encoded_size();
        }
        return size;
    }


#define MAKE_REG(name, upper_name) operand name;
#define MAKE_SEG(name, upper_name)
    namespace reg {
#   include "granary/x86/registers.h"
    }
#undef MAKE_SEG
#undef MAKE_REG


#define MAKE_REG(name, upper_name) \
    STATIC_INITIALISE_ID(register_ ## name, { \
        reg::name = dynamorio::opnd_create_reg(dynamorio::DR_ ## upper_name); \
    })
#define MAKE_SEG(name, upper_name)
#include "granary/x86/registers.h"
#undef MAKE_SEG
#undef MAKE_REG

}

