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

        void mangle_lea(instruction_list_handle in, app_pc) throw();

        void mangle_jump(instruction_list_handle in) throw();
        void mangle_return(instruction_list_handle in) throw();
        void mangle_call(instruction_list_handle in) throw();
        void mangle_indirect_cti(instruction_list_handle in, operand op) throw();

        IF_USER(void mangle_far_memory_refs(instruction_list_handle in,
                                            app_pc estimator_pc) throw();)

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


    /// Defines a data type used to de/mangle an address that may or may not
    /// contain policy-specific bits.
    union mangled_address {

        enum {
            POLICY_NUM_BITS = 8U
        };

        /// The mangled address in terms of the policy and address components.
        /// Note: order of these fields is significant.
        struct {
            uint8_t policy_id:POLICY_NUM_BITS; // low
            uint64_t _:(64 - POLICY_NUM_BITS);
        } as_policy_address __attribute__((packed));

        /// Used to extract "address space" high-order bits for recovering a
        /// native address.
        struct {
            uint64_t _:(64 - POLICY_NUM_BITS);
            uint8_t lost_bits:POLICY_NUM_BITS; // high
        } as_recovery_address __attribute__((packed));

        /// The mangled address as an actual address.
        app_pc as_address;

        /// The mangled address as an unsigned int, which is convenient for
        /// bit masking.
        int64_t as_int;

        mangled_address(void) throw()
            : as_int(0L)
        { }

        mangled_address(app_pc addr_, instrumentation_policy policy_) throw()
            : as_address(addr_)
        {
            as_int <<= POLICY_NUM_BITS;
            as_policy_address.policy_id = policy_.policy_id;
        }

    } __attribute__((packed));

}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
