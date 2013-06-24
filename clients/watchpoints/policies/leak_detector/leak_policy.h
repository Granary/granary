/*
 * leak_policy.h
 *
 *  Created on: 2013-06-21
 *      Author: akshayk
 */

#ifndef WATCHPOINT_BOUND_POLICY_H_
#define WATCHPOINT_BOUND_POLICY_H_


#include "clients/watchpoints/policies/leak_detector/policy_enter.h"
//#include "clients/watchpoints/policies/leak_detector/policy_continue.h"
//#include "clients/watchpoints/policies/leak_detector/policy_exit.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::leak_policy_enter())
#endif

namespace client { namespace wp{

    /// This function is called on entry to instrumented code.
    static void on_enter_code_cache(void) throw() {
        granary::printf("Entering the code cache.\n");
    }


    /// This function is called when we exit from the code cache.
    static void on_exit_code_cache(void) throw() {
        granary::printf("Exiting the code cache.\n");
    }

    }
}

#endif /* WATCHPOINT_BOUND_POLICY_H_ */
