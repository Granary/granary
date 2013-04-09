/*
 * watchpoint.h
 *
 *  Created on: 2013-04-08
 *      Author: akshayk
 */

#ifndef WATCHPOINT_H_
#define WATCHPOINT_H_

#include "granary/client.h"
#include "clients/watchpoint/watchpoint_util.h"

#define GRANARY_INIT_POLICY (client::watchpoint_policy())

namespace client {

    struct watchpoint_policy : public granary::instrumentation_policy {
    public:

        /// Instruction a basic block.
        granary::instrumentation_policy visit_basic_block(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();

        void watchpoint_memory_operations(
        		granary::cpu_state_handle &cpu,
        		granary::thread_state_handle &thread,
        		granary::basic_block_state &bb,
        		granary::instruction_list &ls,
        		granary::instruction &in,
        		dynamorio::app_pc,
        		bool is_write);
    };
}


#endif /* WATCHPOINT_H_ */
