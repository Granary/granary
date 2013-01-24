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

    /// Defines an instruction list mangler. This is responsible for
    /// re-structuring instruction lists to make them safe to emit. Making them
    /// safe to emit involves changing branches/jumps, making sure things are
    /// patchable, etc.
    struct instruction_list_mangler {
    private:

        cpu_state_handle cpu;
        thread_state_handle thread;
        instrumentation_policy policy;
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


    /// Defines a data type used to de/mangle an address that may or may not
    /// contain policy-specific bits.
    union address_mangler {

        /// The mangled address in terms of what is saved/restored on the stack.
        struct {
            uint32_t push1;
            uint32_t push2;
        } as_components  __attribute__((packed));

        /// The mangled address in terms of the policy and address components.
        struct {
            uint64_t address_low:56;
            uint8_t policy_id:8;
        } as_policy_address __attribute__((packed));

        /// The mangled address as an actual address.
        app_pc as_address;

        /// The mangled address as an unsigned int, which is convenient for
        /// bit masking.
        int64_t as_int;

    } __attribute__((packed));

}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
