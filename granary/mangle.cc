/*
 * mangle.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "globals.h"
#include "state.h"
#include "instruction.h"

extern "C" {

    extern void granary_asm_direct_branch(void);

}

namespace granary {


    typedef decltype(instruction_list().first()) instruction_list_handle;


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    static void add_direct_branch_stub(instruction_list &ls,
                                       instruction &in) throw() {


        instruction_list_handle in_first(ls.append(
            push_imm_(int32_(to_application_offset(in.get_cti_target())))));

        instruction_list_handle in_second(ls.append(
            jmp_(pc_(unsafe_cast<app_pc>(granary_asm_direct_branch)))));

        in = call_(instr_(&*in_first));

        // set the state of the instructions to mangled
        in_first->set_mangled();
        in_second->set_mangled();
        in.set_mangled();
        in.set_patchable();
    }


    static void mangle_call(instruction_list &ls, instruction &in) throw() {
        (void) ls;
        (void) in;
        operand target(in.get_cti_target());
        if(dynamorio::opnd_is_pc(target)) {
            add_direct_branch_stub(ls, in);
        }
    }


    static void mangle_return(instruction_list &ls, instruction &in) throw() {
        (void) ls;
        (void) in;
    }


    static void mangle_jump(instruction_list &ls, instruction &in) throw() {
        (void) ls;
        (void) in;
    }


    static void mangle_cli(instruction_list &ls, instruction &in) throw() {
        (void) ls;
        (void) in;
    }


    static void mangle_sti(instruction_list &ls, instruction &in) throw() {
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
                    mangle_call(ls, *in);
                } else if(dynamorio::instr_is_return(*in)) {
                    mangle_return(ls, *in);
                } else {
                    mangle_jump(ls, *in);
                }

            // clear interrupt
            } else if(dynamorio::OP_cli == (*in)->opcode) {
                mangle_cli(ls, *in);

            // restore interrupt
            } else if(dynamorio::OP_sti == (*in)->opcode) {
                mangle_sti(ls, *in);

            } else {
                continue;
            }
        }
    }

}

