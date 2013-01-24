/*
 * instrument.h
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#ifndef INSTRUMENT_H_
#define INSTRUMENT_H_

#include "granary/state.h"
#include "granary/instruction.h"
#include "granary/policy.h"

namespace client {

    struct null_policy {
    public:

        /// Instruction a basic block.
        static granary::instrumentation_policy visit_basic_block(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {

            printf("in null_policy");

            (void) cpu;
            (void) thread;
            (void) bb;
            (void) ls;

            return granary::policy_for<null_policy>();
        }
    };
/*
    template<>
    struct instrument<my_policy> : public granary::client_state {
    public:

        /// Instruction a basic block.
        static granary::instrumentation_policy &
        basic_block(granary::cpu_state_handle &cpu,
                    granary::thread_state_handle &thread,
                    granary::basic_block_state &bb,
                    granary::instruction_list &ls) throw() {

            printf("in my_policy");

            (void) cpu;
            (void) thread;
            (void) bb;
            (void) ls;

            return policy_for<my_policy>();
        }
    };*/
}

#endif /* INSTRUMENT_H_ */
