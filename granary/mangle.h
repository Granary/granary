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

        void mangle_cti(instruction_list_handle in) throw();
        void mangle_direct_cti(instruction_list_handle in, operand op) throw();
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

        /// Get the direct branch lookip (DBL) entry point for a direct operand.
        app_pc dbl_entry_for(
            instrumentation_policy target_policy,
            instruction_list_handle in,
            mangled_address am
        ) throw();


        /// Get the indirect branch lookup (IBL) entry point for an indirect
        /// operand and policy.
        app_pc ibl_entry_for(
            operand target,
            instrumentation_policy policy
        ) throw();


        /// Get the return branch lookup (RBL) entry point for a return address.
        app_pc rbl_entry_for(
            instrumentation_policy policy,
            int num_bytes_to_pop
        ) throw();


        /// Represents the tail of an IBL entry that is common to an IBL and the
        /// slow path of an RBL.
        void ibl_entry_tail(
            instruction_list &ibl,
            instrumentation_policy target_policy
        ) throw();


        /// Emulate the push of a function call's return address onto the stack.
        void emulate_call_ret_addr(instruction_list_handle in) throw();

    public:

        instruction_list_mangler(
            cpu_state_handle &cpu_,
            thread_state_handle &thread_,
            instrumentation_policy &policy_
        ) throw();

        void mangle(instruction_list &ls);
    };
}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
