/*
 * mangle.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/mangle.h"
#include "granary/basic_block.h"
#include "granary/state.h"
#include "granary/detach.h"
#include "granary/policy.h"
#include "granary/register.h"

namespace granary {

    enum {
        _4GB = 4294967296ULL
    };


    /// Computes the absolute value of a pointer difference.
    static uint64_t pc_diff(ptrdiff_t diff) {
        if(diff < 0) {
            return static_cast<uint64_t>(-diff);
        }
        return static_cast<uint64_t>(diff);
    }


    /// Injects N bytes of NOPs into an instuction list after
    /// a specific instruction.
    static void inject_mangled_nops(instruction_list &ls,
                                    instruction_list_handle in,
                                    unsigned num_nops) throw() {
        instruction nop;

        for(; num_nops >= 3; num_nops -= 3) {
            nop = nop3byte_();
            nop.set_mangled();
            ls.insert_after(in, nop);
        }

        for(; num_nops >= 2; num_nops -= 2) {
            nop = nop2byte_();
            nop.set_mangled();
            ls.insert_after(in, nop);
        }

        for(; num_nops >= 1; num_nops -= 1) {
            nop = nop1byte_();
            nop.set_mangled();
            ls.insert_after(in, nop);
        }
    }


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes.
    void stage_8byte_hot_patch(instruction in, app_pc stage, app_pc dest) {
        instruction_list ls;
        ls.append(in);

        const unsigned size(in.encoded_size());
        if(size < 8) {
            inject_mangled_nops(ls, ls.first(), 8U - size);
        }

        ls.stage_encode(stage, dest);
    }


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    void instruction_list_mangler::add_direct_branch_stub(
        instruction_list_handle in,
        operand target
    ) throw() {

        // if this is a detach point then replace the target address with the
        // detach address.
        app_pc detach_target_pc(find_detach_target(target));
        if(nullptr != detach_target_pc) {
            in->set_cti_target(pc_(detach_target_pc));
            in->set_mangled();
            return;
        }

        const unsigned old_size(in->encoded_size());

        instruction_list_handle in_first(ls->append(
            push_imm_(int32_(to_application_offset(target)))));

        instruction_list_handle in_second(ls->append(
            jmp_(pc_(policy.get_direct_cti_patch_func(in->op_code())))));

        *in = call_(instr_(*in_first));

        // set the state of the instructions to mangled
        in_first->set_mangled();
        in_second->set_mangled();
        in->set_mangled();
        in->set_patchable();

        const unsigned new_size(in->encoded_size());

        IF_DEBUG(old_size > 8, FAULT)
        IF_DEBUG(new_size > 8, FAULT)
    }


    /// Add an indirect call slot.
    void instruction_list_mangler::mangle_indirect_call(
        instruction_list_handle in,
        operand target
    ) throw() {

        (void) in;
        (void) target;
    }


    /// Add an indirect jump slot.
    void instruction_list_mangler::mangle_indirect_jump(
        instruction_list_handle in,
        operand target
    ) throw() {

        (void) in;
        (void) target;
    }


    void instruction_list_mangler::mangle_call(
        instruction_list_handle in
    ) throw() {
        operand target(in->cti_target());

        if(dynamorio::opnd_is_pc(target)) {
            add_direct_branch_stub(in, target);
        } else {
            mangle_indirect_call(in, target);
        }
    }


    void instruction_list_mangler::mangle_return(
        instruction_list_handle in
    ) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_jump(
        instruction_list_handle in
    ) throw() {

        operand target(in->cti_target());
        if(dynamorio::opnd_is_pc(target)) {
            add_direct_branch_stub(in, target);
        } else {
            mangle_indirect_jump(in, target);
        }
    }


    void instruction_list_mangler::mangle_cli(instruction_list_handle in) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_sti(instruction_list_handle in) throw() {

        (void) in;
    }


#if CONFIG_TRANSLATE_FAR_ADDRESSES


    /// Detect the use of a relative memory operand whose absolute address
    /// will be too far away from the encoding location, and increment the
    /// vtable counter so that we can spill the absolute address there.
    static void count_far_operands(
        operand_ref op,
        app_pc &estimator_pc,
        unsigned &num_far_ops,
        bool &counted_for_instr
    ) throw() {
        if(counted_for_instr
        || (dynamorio::REL_ADDR_kind != op->kind && dynamorio::PC_kind != op->kind)) {
            return;
        }

        // if the operand is too far away then we need to spill that address to
        // a vtable slot.
        if(_4GB < pc_diff(estimator_pc - op->value.pc)) {
            ++num_far_ops;
            counted_for_instr = true;
        }
    }


    /// Find a far memory operand and its size. If we've already found one in
    /// this instruction then don't repeat the check.
    static void find_far_operand(
        operand_ref op,
        app_pc &estimator_pc,
        operand &far_op,
        bool &has_far_op
    ) throw() {
        if(has_far_op || dynamorio::REL_ADDR_kind != op->kind) {
            return;
        }

        // if the operand is too far away then we will need to spill some
        // registers and use the vtable slot allocated for this address.
        const app_pc addr(reinterpret_cast<app_pc>(op->value.addr));
        if(_4GB >= pc_diff(estimator_pc - addr)) {
            return;
        }

        has_far_op = true;
        far_op = op;
    }


    /// Update a far operand in place.
    static void update_far_operand(operand_ref op, operand &new_op) throw() {
        if(dynamorio::REL_ADDR_kind != op->kind && dynamorio::PC_kind != op->kind) {
            return;
        }

        op = new_op; // update the op in place
    }


    /// Mangle %rip-relative memeory operands into absolute memory operands
    /// (indirectly through a spill register) in user space. This checks to see
    /// if %rip-relative operands and > 4gb away and converts them to a
    /// different form. This is a "two step" process, in that a DR instruction
    /// might have multiple memory operands (e.g. inc, add), and some of them
    /// must all be equivalent.
    ///
    /// We assume it's always legal to convert %rip-relative into a base/disp
    /// type operand (of the same size).
    void instruction_list_mangler::mangle_far_memory_refs(
        instruction_list_handle in,
        app_pc estimator_pc
    ) throw() {

        bool has_far_op(false);
        operand far_op;

        in->for_each_operand(
            find_far_operand, estimator_pc, far_op, has_far_op);

        if(!has_far_op) {
            return;
        }

        // we need to change the instruction; we assume that no instruction
        // has two memory operands. This is sort of correct, in that
        // we can have two memory operands, but they have to match (e.g. inc)
        //
        // Note: because this is a user space thing only, we don't need to be
        //       as concerned about the kind of a byte of an instruction, as
        //       we don't need to deliver precise (or any for that matter)
        //       interrupts. As such, this code is not interrupt safe.

        register_manager rm;
        rm.kill_all();
        rm.revive(*in);

        operand spill_reg(rm.get_zombie());
        const uint64_t addr(reinterpret_cast<uint64_t>(far_op.value.pc));

        ls->insert_before(in, push_(spill_reg));
        ls->insert_after(in, pop_(spill_reg));
        ls->insert_before(in, mov_imm_(spill_reg, int64_(addr)));

        // overwrite the op
        operand_base_disp new_op_(*spill_reg);
        new_op_.size = far_op.size;
        operand new_op(new_op_);

        in->for_each_operand(update_far_operand, new_op);
    }
#endif


    /// Goes through all instructions and adds vtable entries for the direct
    /// calls to far pcs, and indirect calls.
    void instruction_list_mangler::add_vtable_entries(
        unsigned num_needed_vtable_entries,
        app_pc estimator_pc
    ) throw() {

        // allocate the vtable entries in the fragment cache
        array<basic_block_vtable::entry> entries(
            cpu->fragment_allocator.allocate_array<basic_block_vtable::entry>(
                num_needed_vtable_entries));

        // initialize the vtable
        vtable = basic_block_vtable(entries.begin(), num_needed_vtable_entries);

        instruction_list_handle in(ls->first());
        instruction_list_handle next_in;

        for(unsigned i(0), max(ls->length()); i < max; ++i, in = next_in) {
            next_in = in.next();

            // native instruction, we might need to mangle it.
            if(!in->is_cti()) {
                continue;
            }

            operand target(in->cti_target());

            // direct branch (e.g. un/conditional branch, jmp, call)
            if(dynamorio::opnd_is_pc(target)) {
#if CONFIG_TRANSLATE_FAR_ADDRESSES
                app_pc target_pc(dynamorio::opnd_get_pc(target));

                // update the target of the instruction so that it now uses
                // one of the vtable entries.
                if(_4GB < pc_diff(estimator_pc - target_pc)) {

                    printf("adding vtable entry for instruction(%p) targeting (%p)\n", in->pc(), target_pc);

                    basic_block_vtable::address &addr(vtable.next_address());
                    addr.addr = target_pc;
                    in->set_cti_target(absmem_(&(addr.addr), 8));
                }
#endif
            // indirect branch lookup; need vtable entry for caching.
            } else {

            }
        }
    }


    /// Convert non-instrumented instructions that change control-flow into
    /// mangled instructions.
    void instruction_list_mangler::mangle(instruction_list &ls_) throw() {

        ls = &ls_;

        instruction_list_handle in(ls->first());

        // go mangle instructions; note: indirect cti mangling happens here.
        for(unsigned i(0), max(ls->length()); i < max; ++i, in = in.next()) {
            const bool can_skip(nullptr == in->pc() || in->is_mangled());

            // native instruction, we might need to mangle it.
            if(in->is_cti()) {

                if(dynamorio::instr_is_call(*in)) {
                    mangle_call(in);

                } else if(dynamorio::instr_is_return(*in)) {
                    mangle_return(in);

                // JMP, Jcc
                } else {
                    mangle_jump(in);
                }

            // clear interrupt
            } else if(dynamorio::OP_cli == (*in)->opcode) {
                if(can_skip) {
                    continue;
                }
                mangle_cli(in);

            // restore interrupt
            } else if(dynamorio::OP_sti == (*in)->opcode) {
                if(can_skip) {
                    continue;
                }
                mangle_sti(in);
            }
        }

        // used to detect when and how many vtable entries are needed; vtable
        // entries are typically used for far direct jumps and as a cache for
        // indirect jumps, but can also be used in user space for far absolute
        // addresses.
        unsigned num_needed_vtable_entries(0U);
        uint8_t *estimator_pc(
            cpu->fragment_allocator.allocate_staged<uint8_t>());

        // do a second-pass over all instructions, looking for any hot-patchable
        // instructions, and aligning them nicely.
        //
        // Extra alignment/etc needs to be done here instead of in encoding
        // because of how basic block allocation works.
        unsigned align(0);
        instruction_list_handle prev_in;
        instruction_list_handle next_in;
        in = ls->first();

        for(unsigned i(0), max(ls->length()); i < max; ++i, in = next_in) {

            if(in->is_cti()) {
                operand target(in->cti_target());

                // direct branch (e.g. un/conditional branch, jmp, call)
                if(dynamorio::opnd_is_pc(target)) {
#if CONFIG_TRANSLATE_FAR_ADDRESSES
                    bool found_far_op(false);
                    in->for_each_operand(count_far_operands,
                                         estimator_pc,
                                         num_needed_vtable_entries,
                                         found_far_op);
#endif
                // indirect branch lookup; need vtable entry for caching.
                // we assume that the only indirect branches that need a vtable
                // entry are those that map to an application instruction.
                } else if(in->pc()) {
                    ++num_needed_vtable_entries;
                }

            // if in user space, look for uses of relative addresses in operands
            // that are no longer reachable with %rip-relative encoding, and
            // convert to a use of an absolute address (requires spilling a
            // register)
            } else {
                IF_USER(mangle_far_memory_refs(in, estimator_pc);)
            }

            const bool is_hot_patchable(in->is_patchable());

            // x86-64 guaranteed quadword atomic writes so long as
            // the memory location is aligned on an 8-byte boundary;
            // we will assume that we are never patching an instruction
            // longer than 8 bytes
            if(is_hot_patchable) {
                uint64_t forward_align(ALIGN_TO(align, 8));
                inject_mangled_nops(*ls, prev_in, forward_align);
                align += forward_align;
            }

            align += in->encoded_size();
            prev_in = in;
            next_in = in.next();

            // make sure that the instruction is the only "useful" one in it's
            // 8-byte block
            if(is_hot_patchable) {
                uint64_t forward_align(ALIGN_TO(align, 8));
                inject_mangled_nops(ls_, prev_in, forward_align);
                align += forward_align;
            }
        }

        // go add the vtable entries; this is also where indirect cti mangling
        // happens
        if(num_needed_vtable_entries) {
            add_vtable_entries(num_needed_vtable_entries, estimator_pc);
        }
    }


    /// Constructor
    instruction_list_mangler::instruction_list_mangler(
        cpu_state_handle &cpu_,
        thread_state_handle &thread_,
        instrumentation_policy &policy_,
        basic_block_vtable &vtable_
    ) throw()
        : cpu(cpu_)
        , thread(thread_)
        , policy(policy_)
        , vtable(vtable_)
        , ls(nullptr)
    { }

}

