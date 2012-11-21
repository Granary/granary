/*
 * instrument.cc
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#include "clients/instrument.h"

namespace client {

    void instrument(granary::instruction_list &ls,
                    granary::basic_block_state *state,
                    bool interrupt_state) throw() {
        (void) ls;
        (void) state;
        (void) interrupt_state;
    }

}
