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

namespace client {

    template <typename Policy>
    struct instrument {
    public:

        /// Instruction a basic block.
        static void basic_block(granary::cpu_state_handle &cpu,
                                 granary::thread_state_handle &thread,
                                 granary::basic_block_state *bb,
                                 granary::instruction_list &ls) throw() {

           (void) cpu;
           (void) thread;
           (void) bb;
           (void) ls;
        }
    };
}

#endif /* INSTRUMENT_H_ */
