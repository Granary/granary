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

#include "granary/instruction.h"

#include "clients/cfg/config.h"

#if CFG_RECORD_INDIRECT_TARGETS
#   include "granary/spin_lock.h"
#endif

namespace client {


    /// Data recorded about each indirect CTI.
    struct indirect_cti {

        /// The last indirect target. This is a single entry cache.
        granary::app_pc last_indirect_target;

        char padding[CONFIG_ARCH_CACHE_LINE_SIZE - sizeof(granary::app_pc)];

        /// The full set of indirect targets for this
        granary::app_pc *indirect_targets;

        /// The number of indirect targets.
        unsigned num_indirect_targets;

        /// Lock for modifying the number of indirect targets.
        granary::atomic_spin_lock update_lock;

    } __attribute__((aligned (CONFIG_ARCH_CACHE_LINE_SIZE)));


    /// State that is automatically maintained for each instrumented basic
    /// basic block.
    struct basic_block_state {
    public:

        /// An instruction that's encoded into the code cache. This will give
        /// us access (later)
        granary::persistent_instruction label;

        /// Used to chain all constructed basic blocks together for later
        /// iterating / dumping.
        basic_block_state *next;

#if CFG_RECORD_EXEC_COUNT
        /// Number of times this basic block was executed.
        uint64_t num_executions;

#   if CFG_RECORD_FALL_THROUGH_COUNT
        /// Number of times the fall-through of this basic block was
        /// executed.
        uint64_t num_fall_through_executions;
#   endif
#endif

#if CFG_RECORD_INDIRECT_TARGETS
        /// There can be more than one indirect control-flow transfers in a
        /// single basic block. Each one is associated with its own recorded
        /// info.
        indirect_cti *indirect_ctis;

        unsigned num_indirect_ctis;
#endif
    };
}

#endif /* CFG_STATE_H_ */
