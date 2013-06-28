/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */

#ifndef CLIENT_STATE_H_
#define CLIENT_STATE_H_

#if defined(CLIENT_WATCHPOINT_BOUND) \
 || defined(CLIENT_WATCHPOINT_LEAK)
#   include "clients/watchpoints/state.h"
#endif


#ifdef CLIENT_CFG
#   include "clients/cfg/state.h"
#endif

namespace client {

    struct thread_state;
    struct cpu_state;
    struct block_state;


#ifndef CLIENT_thread_state
    /// Extensions to Granary's internal thread-local storage.
    struct thread_state {

    };
#endif /* CLIENT_thread_state */


#ifndef CLIENT_cpu_state
    /// Extensions to Granary's internal CPU-local storage.
    struct cpu_state {
#   if GRANARY_IN_KERNEL

#   endif
    };
#endif /* CLIENT_cpu_state */


#ifndef CLIENT_basic_block_state
    /// Extensions to Granary's internal basic block-local storage.
    struct basic_block_state { };
#endif /* CLIENT_basic_block_state */

}

#endif /* CLIENT_STATE_H_ */
