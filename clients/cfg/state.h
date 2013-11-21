/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef CFG_STATE_H_
#define CFG_STATE_H_

#define CLIENT_basic_block_state
#define CLIENT_commit_to_basic_block_state

#include "granary/register.h"
#include "granary/spin_lock.h"

#include "clients/cfg/config.h"

namespace client {


    /// Forward declaration.
    struct basic_block_state;


    /// State that is automatically maintained for each instrumented basic
    /// basic block.
    struct basic_block_state {
    public:

        /// An instruction that's encoded into the code cache. This will give
        /// us access (later)
        dynamorio::instr_t label;

        /// Used to chain all constructed basic blocks together for later
        /// iterating / dumping.
        basic_block_state *next;

#if CFG_RECORD_EXEC_COUNT
        /// Number of times this basic block was executed.
        uint64_t num_executions;
#endif

        /// Outgoing edges to indirect CTI targets.
        basic_block_state *indirect_edges;
    };
}

#endif /* CFG_STATE_H_ */
