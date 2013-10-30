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
        basic_block_state *bb;
        const instrumentation_policy policy;
        instruction_list *ls;
        instruction_list *stub_ls;

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

    public:

        static void insert_nops_after(
            instruction_list &ls,
            instruction in,
            unsigned num_nops
        ) throw();

        static void stage_8byte_hot_patch(
            instruction in,
            app_pc stage,
            app_pc dest,
            unsigned offset=0
        ) throw();

    private:

#if CONFIG_TRANSLATE_FAR_ADDRESSES

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
#endif

        /// Get the direct branch lookip (DBL) entry point for a direct operand.
        app_pc dbl_entry_routine(
            instrumentation_policy target_policy,
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
        /// canonical way for entry into the indirect branch lookup table.
        app_pc ibl_pre_entry_routine(
            instrumentation_policy target_policy,
            operand target,
            ibl_entry_kind ibl_kind
        ) throw();


        /// Return the IBL entry routine. The IBL entry routine is responsible
        /// for looking to see if an address (stored in reg::arg1) is located
        /// in the CPU-private code cache or in the global code cache. If the
        /// address is in the CPU-private code cache, and if IBL prediction is
        /// enabled, then the CPU-private lookup function might add a prediction
        /// entry to the CTI.
        static app_pc ibl_entry_routine(
            instrumentation_policy target_policy
        ) throw();


        /// Return or generate the IBL exit routine for a particular jump target.
        /// The target can either be code cache or native code.
        static app_pc ibl_exit_routine(
            app_pc target_pc
        );


    public:


#if 0 && !CONFIG_ENABLE_DIRECT_RETURN
        /// Checks to see if a return address is in the code cache. If so, it
        /// RETs to the address, otherwise it JMPs to the IBL entry routine.
        app_pc rbl_entry_routine(
            instrumentation_policy target_policy
        ) throw();
#endif


        instruction_list_mangler(
            cpu_state_handle cpu_,
            basic_block_state *bb_,
            instrumentation_policy &policy_
        ) throw();


        void mangle(instruction_list &ls, instruction_list &stub_ls);
    };
}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
