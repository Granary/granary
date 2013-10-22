/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: 2013-10-21
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_USER_STATE_H_
#define WATCHPOINT_USER_STATE_H_

namespace client {

/// Select the features that we want to use.
#define CLIENT_basic_block_state
#define CLIENT_commit_to_basic_block_state


    struct basic_block_state {

        void *instruction_address;

        std::atomic<basic_block_state *> next;

        bool accesses_user_data;
    };
}

#endif /* WATCHPOINT_USER_STATE_H_ */
