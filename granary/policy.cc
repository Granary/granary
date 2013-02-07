/*
 * policy.cc
 *
 *  Created on: 2013-01-24
 *      Author: pag
 */

#include "granary/policy.h"
#include "granary/mangle.h"

namespace granary {


    /// Policy basic block visitor functions for each policy. The code
    /// cache will use this array of function pointers (initialised
    /// partially at compile time and partially at run time) to determine
    /// which client-code basic block visitor functions should be called.
    instrumentation_policy::basic_block_visitor *
    instrumentation_policy::POLICY_FUNCTIONS[
        1 << mangled_address::POLICY_NUM_BITS
    ] = {
        &(instrumentation_policy::missing_policy)
    };


    /// Policy ID tracker.
    std::atomic<unsigned> instrumentation_policy::NEXT_POLICY_ID(
        ATOMIC_VAR_INIT(instrumentation_policy::NUM_POLICIES));


    /// Instrumentation policy for basic blocks where the policy is missing.
    struct missing_policy_policy {
    public:

        /// Instruction a basic block.
        static instrumentation_policy visit_basic_block(
            cpu_state_handle &,
            thread_state_handle &,
            basic_block_state &,
            instruction_list &
        ) throw() {
            return granary::policy_for<missing_policy_policy>();
        }
    };


    /// Function called when a policy is missing (i.e. hasn't been initialised).
    instrumentation_policy instrumentation_policy::missing_policy(
        cpu_state_handle &,
        thread_state_handle &,
        basic_block_state &,
        instruction_list &
    ) throw() {
        granary_break_on_fault();
        granary_fault();
        return policy_for<missing_policy_policy>();
    }

}

