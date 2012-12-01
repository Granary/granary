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


    static bool mangled_nops(false);
    static instruction nop3(nop3byte_());
    static instruction nop2(nop2byte_());
    static instruction nop1(nop1byte_());


    /// Injects N bytes of NOPs into an instuction list after
    /// a specific instruction. This is similar to encode_mangled_
    /// nops in instruction.cc.
    static void inject_mangled_nops(instruction_list &ls,
                                      instruction_list_handle in,
                                      unsigned num_nops) throw() {

        if(!mangled_nops) {
            mangled_nops = true;
            nop3.set_mangled();
            nop2.set_mangled();
            nop1.set_mangled();
        }

        for(; num_nops >= 3; num_nops -= 3) {
            ls.insert_after(in, nop3);
        }

        for(; num_nops >= 2; num_nops -= 2) {
            ls.insert_after(in, nop2);
        }

        for(; num_nops >= 1; num_nops -= 1) {
            ls.insert_after(in, nop1);
        }
    }


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes.
    void stage_8byte_hot_patch(instruction in, app_pc stage, app_pc dest) {
        instruction_list ls;

        const unsigned size(in.encoded_size());

        ls.append(in);
        if(size < 8) {
            inject_mangled_nops(ls, ls.first(), 8U - size);
        }

        ls.stage_encode(stage, dest);
    }


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    static void add_direct_branch_stub(instrumentation_policy &policy,
                                          instruction_list &ls,
                                          instruction_list_handle in,
                                          operand target) throw() {

        // don't mangle this slot; it is a detach point.
        if(nullptr != find_detach_target(target)) {
            return;
        }

        const unsigned old_size(in->encoded_size());

        instruction_list_handle in_first(ls.append(
            push_imm_(int32_(to_application_offset(target)))));

        instruction_list_handle in_second(ls.append(
            jmp_(pc_(policy.get_direct_cti_patch_func(in->op_code())))));

        *in = call_(instr_(*in_first));

        // set the state of the instructions to mangled
        in_first->set_mangled();
        in_second->set_mangled();
        in->set_mangled();
        in->set_patchable();

        // pad the size out to 8 bytes using NOPs
        const unsigned new_size(in->encoded_size());
        if(new_size < 8U) {
            inject_mangled_nops(ls, in, 8U - new_size);
        }

        IF_DEBUG(old_size > 8, FAULT)
        IF_DEBUG(new_size > 8, FAULT)
    }


    /// Add an indirect jump test slot.
    static void mangle_indirect_call(instrumentation_policy &policy,
                                        instruction_list &ls,
                                        instruction_list_handle in,
                                        operand target) throw() {

        (void) policy;
        (void) ls;
        (void) in;
        (void) target;
    }


    static void mangle_call(instrumentation_policy &policy,
                             instruction_list &ls,
                             instruction_list_handle in) throw() {
        operand target(in->cti_target());

        if(dynamorio::opnd_is_pc(target)) {
            if(!find_detach_target(target.value.pc)) {
                add_direct_branch_stub(policy, ls, in, target);
            }
        } else {
            mangle_indirect_call(policy, ls, in, target);
        }
    }


    static void mangle_return(instrumentation_policy &policy,
                                instruction_list &ls,
                                instruction_list_handle in) throw() {
        (void) policy;
        (void) ls;
        (void) in;
    }


    static void mangle_jump(instrumentation_policy &policy,
                             instruction_list &ls,
                             instruction_list_handle in) throw() {
        operand target(in->cti_target());
        if(dynamorio::opnd_is_pc(target)) {
            if(!find_detach_target(target.value.pc)) {
                add_direct_branch_stub(policy, ls, in, target);
            }
        }
    }


    static void mangle_cli(instrumentation_policy &policy,
                            instruction_list &ls,
                            instruction_list_handle in) throw() {
        (void) policy;
        (void) ls;
        (void) in;
    }


    static void mangle_sti(instrumentation_policy &policy,
                            instruction_list &ls,
                            instruction_list_handle in) throw() {
        (void) policy;
        (void) ls;
        (void) in;
    }


    /// Convert non-instrumented instructions that change control-flow into
    /// mangled instructions.
    void mangle(instrumentation_policy &policy,
                 cpu_state_handle &cpu,
                 thread_state_handle &thread,
                 instruction_list &ls) throw() {

        (void) cpu;
        (void) thread;

        instruction_list_handle in(ls.first());

        for(unsigned i(0), max(ls.length()); i < max; ++i, in = in.next()) {
            const bool can_skip(nullptr == in->pc() || in->is_mangled());

            // native instruction, we might need to mangle it.
            if(in->is_cti()) {

                if(dynamorio::instr_is_call(*in)) {
                    mangle_call(policy, ls, in);

                } else if(dynamorio::instr_is_return(*in)) {
                    mangle_return(policy, ls, in);

                // JMP, Jcc
                } else {
                    mangle_jump(policy, ls, in);
                }

            // clear interrupt
            } else if(dynamorio::OP_cli == (*in)->opcode) {
                if(can_skip) {
                    continue;
                }
                mangle_cli(policy, ls, in);

            // restore interrupt
            } else if(dynamorio::OP_sti == (*in)->opcode) {
                if(can_skip) {
                    continue;
                }
                mangle_sti(policy, ls, in);
            }
        }
    }

}

