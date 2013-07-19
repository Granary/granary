/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: 2013-07-18
 *      Author: Peter Goodman
 */

#ifndef CLIENT_WATCHPOINT_STATS_STATE_H_
#define CLIENT_WATCHPOINT_STATS_STATE_H_

#define CLIENT_basic_block_state
#define CLIENT_commit_to_basic_block_state

namespace client {

    struct basic_block_state {
        basic_block_state *next;

        uint64_t num_memory_ops;
        uint64_t num_executions;
        uint64_t num_watched_memory_ops;
    };
}

#endif /* CLIENT_WATCHPOINT_STATS_STATE_H_ */
