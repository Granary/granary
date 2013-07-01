/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef CFG_EVENTS_H_
#define CFG_EVENTS_H_

#include "granary/client.h"

namespace client {


    /// Invoked when we enter into a basic block targeted by a CALL instruction.
    void event_enter_function(basic_block_state *) throw();


    /// Invoked when we enter into a basic block targeted by a CALL instruction.
    void event_exit_function(basic_block_state *) throw();


    /// Invoked when we enter into a basic block that is not targeted by a CALL
    /// instruction.
    void event_enter_basic_block(basic_block_state *) throw();
}

#endif /* CFG_EVENTS_H_ */
