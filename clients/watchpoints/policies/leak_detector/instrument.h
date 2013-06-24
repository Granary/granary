/*
 * leak_policy.h
 *
 *  Created on: 2013-06-21
 *      Author: akshayk, pgoodman
 */

#ifndef WATCHPOINT_BOUND_POLICY_H_
#define WATCHPOINT_BOUND_POLICY_H_


#include "clients/watchpoints/instrument.h"


#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::leak_policy_enter())
#endif


/// Included after `GRANARY_INIT_POLICY` so that `watchpoint_null` is not the
/// default policy.
#include "clients/watchpoints/policies/null_policy.h"

namespace client {

    struct leak_policy_enter : public watchpoint_null_policy {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };

    struct leak_policy_continue : public watchpoint_null_policy {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };

    struct leak_policy_exit : public watchpoint_null_policy {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };

}

#endif /* WATCHPOINT_BOUND_POLICY_H_ */
