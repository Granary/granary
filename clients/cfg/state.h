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
#define CLIENT_thread_state

#include "granary/register.h"
#include "granary/smp/spin_lock.h"

namespace client {


    /// State that is automatically maintained for each instrumented basic
    /// basic block.
    struct basic_block_state {
    public:

        /// Used to chain all constructed basic blocks together for later
        /// iterating / dumping.
        basic_block_state *next;

        /// Conservative set of live registers on entry to this basic block.
        granary::register_manager entry_regs;


        /// Set of all used registers in this basic block.
        granary::register_manager used_regs;


#if GRANARY_IN_KERNEL
        /// Name of the module being instrumented.
        const char *app_name;

        /// Offset within that module's `.text` section.
        unsigned app_offset_begin;
        unsigned app_offset_end;

        /// Number of times this basic block was interrupted.
        std::atomic<unsigned> num_interrupts;
#endif

        /// Number of times this basic block was executed.
        std::atomic<unsigned> num_executions;

        /// Function ID of this basic block.
        unsigned function_id;

        enum {
            NUM_INCOMING_EDGES = 3
        };

        /// Basic blocks within the same function that lead to this basic block.
        basic_block_state *local_incoming[NUM_INCOMING_EDGES];

        /// Lock that is acquired when updating the graph from this block.
        granary::smp::atomic_spin_lock update_lock;

        /// Is this basic block an entry/exit basic block for a function?
        bool is_function_entry;
        bool is_function_exit;
    };


    /// State that is automatically maintained for each thread.
    struct thread_state {

        /// The most recently executed basic block. This is used for
        /// constructing various intra-procedural control-flow graphs.
        client::basic_block_state *last_executed_basic_block;
    };
}

#endif /* CFG_STATE_H_ */
