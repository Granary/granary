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
    void event_enter_function(granary::basic_block_state *) throw();


    /// Invoked when we enter into a basic block that is not targeted by a CALL
    /// instruction.
    void event_enter_basic_block(granary::basic_block_state *) throw();


    /// Invoked before we make an indirect call.
    void event_call_indirect(granary::basic_block_state *) throw();


    /// Invoked before we call app code.
    void event_call_app(granary::basic_block_state *) throw();


    /// Invoked before we call host code.
    void event_call_host(granary::basic_block_state *, granary::app_pc) throw();


    /// Invoked when we return from a function call.
    void event_return_from_call(granary::basic_block_state *) throw();
}

#endif /* CFG_EVENTS_H_ */
