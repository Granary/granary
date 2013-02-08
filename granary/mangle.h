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


    /// Defines an instruction list mangler. This is responsible for
    /// re-structuring instruction lists to make them safe to emit. Making them
    /// safe to emit involves changing branches/jumps, making sure things are
    /// patchable, etc.
    struct instruction_list_mangler {
    private:

        cpu_state_handle cpu;
        thread_state_handle thread;
        instrumentation_policy policy;
        instruction_list *ls;

        void mangle_sti(instruction_list_handle in) throw();
        void mangle_cli(instruction_list_handle in) throw();

        void mangle_jump(instruction_list_handle in) throw();
        void mangle_return(instruction_list_handle in) throw();
        void mangle_call(instruction_list_handle in) throw();
        void mangle_indirect_cti(instruction_list_handle in, operand op) throw();

        void propagate_delay_region(
            instruction_list_handle in,
            instruction_list_handle first,
            instruction_list_handle last
        ) throw();

#if CONFIG_TRANSLATE_FAR_ADDRESSES

        void mangle_lea(instruction_list_handle in, app_pc) throw();

        void mangle_far_memory_refs(
            instruction_list_handle in,
            app_pc estimator_pc
        ) throw();

        void mangle_far_memory_push(
            instruction_list_handle in,
            bool first_reg_is_dead,
            dynamorio::reg_id_t dead_reg_id,
            dynamorio::reg_id_t spill_reg_id,
            uint64_t addr
        ) throw();

        void mangle_far_memory_pop(
            instruction_list_handle in,
            bool first_reg_is_dead,
            dynamorio::reg_id_t dead_reg_id,
            dynamorio::reg_id_t spill_reg_id,
            uint64_t addr
        ) throw();
#endif

        void add_direct_branch_stub(instruction_list_handle in, operand op) throw();


        /// Get the IBL entry point for an indirect operand and policy.
        app_pc ibl_entry_for(
            operand target,
            instrumentation_policy policy
        ) throw();

    public:

        instruction_list_mangler(cpu_state_handle &cpu_,
                                 thread_state_handle &thread_,
                                 instrumentation_policy &policy_) throw();

        void mangle(instruction_list &ls);
    };
}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
