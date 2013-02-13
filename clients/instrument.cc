/*
 * instrument.cc
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#include "clients/instrument.h"

namespace client {

    /// Instruction a basic block.
    granary::instrumentation_policy null_policy::visit_basic_block(
        granary::cpu_state_handle &cpu,
        granary::thread_state_handle &thread,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {

        //granary::printf("pc = %p\n", ls.first()->pc());

        (void) cpu;
        (void) thread;
        (void) bb;
        (void) ls;

        return granary::policy_for<null_policy>();
    }

}
