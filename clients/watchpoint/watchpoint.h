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

//#define GRANARY_INIT_POLICY (client::watchpoint_policy())

namespace client {

    struct watchpoint_policy : public granary::instrumentation_policy {
    private:
        void instrumentation_base_disp(granary::instruction_list &ls,
                granary::instruction &in,
                dynamorio::app_pc pc,
                client::instruction_util &ops,
                bool is_write);

        void instrumentation_far_base_disp(granary::instruction_list &ls,
                granary::instruction &in,
                dynamorio::app_pc pc,
                client::instruction_util &ops,
                bool is_write);

        void instrumentation_operand_rep(granary::instruction_list &ls,
                granary::instruction &in,
                dynamorio::app_pc pc,
                client::instruction_util &ops,
                bool is_write);

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
                dynamorio::app_pc pc,
                bool is_write);

        void check_pc(dynamorio::app_pc pc){
            (void)pc;
        }

        enum {
            WATCHPOINT_INDEX_MASK   = 0xffff800000000000ULL,
            SHADOW_START_ADDR       = 0xffffffffe0000000ULL,
            SHADOW_END_ADDR         = 0xffffffffff000000ULL,
            SHIFT_BIT_COUNT         = 0x30
        };

    };


}


#endif /* WATCHPOINT_H_ */
