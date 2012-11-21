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

    void instrument(granary::instruction_list &ls,
                    granary::basic_block_state *state,
                    bool interrupt_state) throw();

}

#endif /* INSTRUMENT_H_ */
