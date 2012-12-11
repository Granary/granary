/*
 * mangle.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/mangle.h"
#include "granary/state.h"
#include "granary/detach.h"
#include "granary/policy.h"


namespace granary {


    typedef decltype(instruction_list().first()) instruction_list_handle;


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

        // don't mangle this slot; it is a detach point.
        if(nullptr != find_detach_target(target)) {
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


    /// Add an indirect jump test slot.
    void instruction_list_mangler::mangle_indirect_call(
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
            if(!find_detach_target(target.value.pc)) {
                add_direct_branch_stub(in, target);
            }
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
            if(!find_detach_target(target.value.pc)) {
                add_direct_branch_stub(in, target);
            }
        }
    }


    void instruction_list_mangler::mangle_cli(instruction_list_handle in) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_sti(instruction_list_handle in) throw() {

        (void) in;
    }


    /// Convert non-instrumented instructions that change control-flow into
    /// mangled instructions.
    void instruction_list_mangler::mangle(instruction_list &ls_) throw() {

        ls = &ls_;

        (void) cpu;
        (void) thread;

        instruction_list_handle in(ls->first());

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
    }


    /// Constructor
    instruction_list_mangler::instruction_list_mangler(
        cpu_state_handle &cpu_,
        thread_state_handle &thread_,
        instrumentation_policy &policy_
    ) throw()
        : cpu(cpu_)
        , thread(thread_)
        , policy(policy_)
        , ls(nullptr)
    { }

}

