/*
 * mangle.h
 *
 *  Created on: 2012-11-27
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_MANGLE_H_
#define Granary_MANGLE_H_

#include "granary/instruction.h"
#include "granary/state.h"

namespace granary {

    /// Forward declarations
    struct basic_block_vtable;
    struct instrumentation_policy;

    /// Defines an instruction list mangler. This is responsible for
    /// re-structuring instruction lists to make them safe to emit. Making them
    /// safe to emit involves changing branches/jumps, making sure things are
    /// patchable, etc.
    struct instruction_list_mangler {
    private:

        cpu_state_handle cpu;
        thread_state_handle thread;
        instrumentation_policy &policy;
        basic_block_vtable &vtable;
        instruction_list *ls;

        void mangle_sti(instruction_list_handle in) throw();
        void mangle_cli(instruction_list_handle in) throw();

        void mangle_jump(instruction_list_handle in) throw();
        void mangle_return(instruction_list_handle in) throw();
        void mangle_call(instruction_list_handle in) throw();
        void mangle_indirect_call(instruction_list_handle in, operand op) throw();
        void mangle_indirect_jump(instruction_list_handle in, operand op) throw();

        IF_USER(void mangle_far_memory_refs(instruction_list_handle in,
                                            app_pc estimator_pc) throw();)

        void add_direct_branch_stub(instruction_list_handle in, operand op) throw();

        void add_vtable_entries(unsigned num_needed_vtable_entries,
                                app_pc estimator_pc) throw();

    public:

        instruction_list_mangler(cpu_state_handle &cpu_,
                                 thread_state_handle &thread_,
                                 instrumentation_policy &policy_,
                                 basic_block_vtable &vtable_) throw();

        void mangle(instruction_list &ls);
    };


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes.
    void stage_8byte_hot_patch(instruction in, app_pc stage, app_pc dest);

}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
