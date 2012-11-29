/*
 * mangle.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/mangle.h"
#include "granary/state.h"
#include "granary/detach.h"


/// Force an instantiation of a particular template.
#define INSTANTIATE_DIRECT_JUMP_MANGLER(opcode, size) \
    template void patch_mangled_direct_cti<opcode ## _>(direct_patch_mcontext *);


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DECLARE_DIRECT_JUMP_MANGLER(opcode, size) \
    extern void granary_asm_direct_branch_ ## opcode(void);


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define CASE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: return granary_asm_direct_branch_ ## opcode;


extern "C" {
    FOR_EACH_DIRECT_BRANCH(DECLARE_DIRECT_JUMP_MANGLER)
}


namespace granary {


    FOR_EACH_DIRECT_BRANCH(INSTANTIATE_DIRECT_JUMP_MANGLER)


    typedef void (direct_cti_patch_func)(void);
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


    /// Look up and return the assembly patch (see asm/direct_branch.asm)
    /// function needed to patch an instruction that originally had opcode as
    /// `opcode`.
    static direct_cti_patch_func *
    get_direct_cti_patch_func(int opcode) throw() {
        switch(opcode) {
        FOR_EACH_DIRECT_BRANCH(CASE_DIRECT_JUMP_MANGLER);
        default: return nullptr;
        }
    }


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    static void add_direct_branch_stub(instruction_list &ls,
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
            jmp_(pc_(unsafe_cast<app_pc>(
                get_direct_cti_patch_func(in->op_code()))))));

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


    static void mangle_call(instruction_list &ls,
                             instruction_list_handle in) throw() {
        operand target(in->cti_target());
        if(dynamorio::opnd_is_pc(target)) {
            add_direct_branch_stub(ls, in, target);
        }
    }


    static void mangle_return(instruction_list &ls,
                                instruction_list_handle in) throw() {
        (void) ls;
        (void) in;
    }


    static void mangle_jump(instruction_list &ls,
                             instruction_list_handle in) throw() {
        operand target(in->cti_target());
        if(dynamorio::opnd_is_pc(target)) {
            add_direct_branch_stub(ls, in, target);
        }
    }


    static void mangle_loop(instruction_list &ls,
                              instruction_list_handle in) throw() {
        operand target(in->cti_target());
        instruction_list_handle in_first(ls.insert_before(in,
            jmp_(instr_(*in))));
        instruction_list_handle in_second(ls.insert_before(in,
            jmp_(target)));

        in->set_cti_target(instr_(*in_second));
        in->set_mangled();
        in_first->set_mangled();
        in_second->set_pc(in->pc());

        // recursively mangle the jump
        mangle_jump(ls, in_second);
    }


    static void mangle_cli(instruction_list &ls,
                            instruction_list_handle in) throw() {
        (void) ls;
        (void) in;
    }


    static void mangle_sti(instruction_list &ls,
                            instruction_list_handle in) throw() {
        (void) ls;
        (void) in;
    }


    /// Convert non-instrumented instructions that change control-flow into
    /// mangled instructions.
    void mangle(cpu_state_handle &cpu,
                 thread_state_handle &thread,
                 instruction_list &ls) throw() {

        (void) cpu;
        (void) thread;

        instruction_list_handle in(ls.first());

        for(unsigned i(0), max(ls.length()); i < max; ++i, in = in.next()) {
            if(nullptr == in->pc() || in->is_mangled()) {
                continue;
            }

            // native instruction, we might need to mangle it.
            if(in->is_cti()) {

                if(dynamorio::instr_is_call(*in)) {
                    mangle_call(ls, in);
                } else if(dynamorio::instr_is_return(*in)) {
                    mangle_return(ls, in);

                // LOOP, LOOPcc, JCXZ, JECXZ, JRCXZ
                } else if(dynamorio::instr_is_cti_loop(*in)) {
                    mangle_loop(ls, in);

                // JMP, Jcc
                } else {
                    mangle_jump(ls, in);
                }

            // clear interrupt
            } else if(dynamorio::OP_cli == (*in)->opcode) {
                mangle_cli(ls, in);

            // restore interrupt
            } else if(dynamorio::OP_sti == (*in)->opcode) {
                mangle_sti(ls, in);

            } else {
                continue;
            }
        }
    }

}

