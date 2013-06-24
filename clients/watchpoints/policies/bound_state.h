/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_state.h
 *
 *  Created on: 2013-06-13
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_BOUND_STATE_H_
#define WATCHPOINT_BOUND_STATE_H_

namespace client {

    namespace wp {
        struct bound_descriptor;
    }


#define CLIENT_cpu_state
    struct cpu_state {

        /// List of free bounds checking objects for this CPU.
        wp::bound_descriptor *free_list;
    };

}

#endif /* WATCHPOINT_BOUND_STATE_H_ */
