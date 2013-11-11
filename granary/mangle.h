/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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


    /// Forward declarations.
    struct code_cache;
    struct prediction_table;


    /// Defines an instruction list mangler. This is responsible for
    /// re-structuring instruction lists to make them safe to emit. Making them
    /// safe to emit involves changing branches/jumps, making sure things are
    /// patchable, etc.
    struct instruction_list_mangler {
    private:

        friend struct code_cache;

        cpu_state_handle cpu;
        basic_block_state &bb;
        const instrumentation_policy policy;
        instruction_list &ls;
        instruction_list &stub_ls;

        // used to estimate if an address is too far away from the code cache
        // to use relative addressing.
        const const_app_pc estimator_pc;

        instruction dbl_entry_stub(
            instruction patched_in,
            app_pc dbl_routine
        ) throw();

        void mangle_sti(instruction in) throw();
        void mangle_cli(instruction in) throw();

        void mangle_cti(instruction in) throw();

        void mangle_direct_cti(
            instruction in,
            operand target,
            instrumentation_policy target_policy
        ) throw();

        void mangle_indirect_cti(
            instruction in,
            operand op,
            instrumentation_policy target_policy
        ) throw();

        void mangle_bit_scan(instruction in) throw();

        static void propagate_delay_region(
            instruction in,
            instruction first,
            instruction last
        ) throw();

    private:

        void mangle_lea(instruction in) throw();

        void mangle_far_memory_refs(instruction in) throw();

        void mangle_far_memory_push(
            instruction in,
            bool first_reg_is_dead,
            dynamorio::reg_id_t dead_reg_id,
            dynamorio::reg_id_t spill_reg_id,
            uint64_t addr
        ) throw();

        void mangle_far_memory_pop(
            instruction in,
            bool first_reg_is_dead,
            dynamorio::reg_id_t dead_reg_id,
            dynamorio::reg_id_t spill_reg_id,
            uint64_t addr
        ) throw();


        /// Get the direct branch lookip (DBL) entry point for a direct operand.
        app_pc dbl_entry_routine(
            instruction in,
            mangled_address am
        ) throw();


        enum ibl_entry_kind {
            IBL_ENTRY_CALL,
            IBL_ENTRY_RETURN,
            IBL_ENTRY_JMP
        };


        /// Make an IBL stub. This is used by indirect jmps, calls, and returns.
        /// The purpose of the stub is to set up the registers and stack in a
        /// canonical way for entry into the indirect branch lookup routine.
        void mangle_ibl_lookup(
            instruction_list &insertion_list,
            instruction insertion_point,
            instrumentation_policy target_policy,
            operand target,
            ibl_entry_kind ibl_kind,
            app_pc cti_in
        ) throw();


    public:


        instruction_list_mangler(
            cpu_state_handle cpu_,
            basic_block_state &bb_,
            instruction_list &ls_,
            instruction_list &stub_ls_,
            instrumentation_policy policy_,
            const_app_pc
        ) throw();


        void mangle(void) throw();


        /// Aligns hot-patchable instructions in the basic block, given the
        /// current cache-line alignment of the basic block, and returns the
        /// basic block's size.
        static unsigned align(instruction_list &ls, unsigned align) throw();
    };
}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
