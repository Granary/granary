/*
 * instrument.h
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#ifndef INSTRUMENT_H_
#define INSTRUMENT_H_

#include "granary/client.h"

namespace client {

    struct null_policy : public granary::instrumentation_policy {
    public:

        /// Instruction a basic block.
        granary::instrumentation_policy visit_basic_block(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };
}

#endif /* INSTRUMENT_H_ */
