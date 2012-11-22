/*
 * instrument.cc
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#include "clients/instrument.h"

namespace client {

    void instrument(granary::cpu_state_handle &cpu,
                    granary::thread_state_handle &thread,
                    granary::basic_block_state *bb,
                    granary::instruction_list &ls) throw() {
        (void) cpu;
        (void) thread;
        (void) bb;
        (void) ls;
    }

}
