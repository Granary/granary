/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.cc
 *
 *  Created on: 2013-07-18
 *      Author: Peter Goodman
 */

#include <atomic>

#include "clients/watchpoints/clients/stats/instrument.h"


using namespace granary;


namespace client {


    /// Head of the basic blocks linked list.
    extern std::atomic<granary::basic_block_state *> BASIC_BLOCKS;


    /// Report on watchpoints statistics.
    void report(void) {

        uint64_t num_bbs(0);
        uint64_t num_bbs_with_watched_ops(0);
        uint64_t num_executed_bbs(0);
        uint64_t num_static_ops(0);
        uint64_t num_dynamic_ops(0);
        unsigned num_watched_ops(0);

        basic_block_state *bb(BASIC_BLOCKS.load());
        for(; bb; bb = bb->next) {
            num_bbs++;
            num_static_ops += bb->num_memory_ops;
            num_executed_bbs += bb->num_executions;
            num_dynamic_ops += bb->num_executions * bb->num_memory_ops;
            num_watched_ops += bb->num_watched_memory_ops;

            if(bb->num_watched_memory_ops) {
                num_bbs_with_watched_ops++;
            }
        }

        printf("\nWatchpoint Statistics\n");

        printf("Number of basic blocks: %lu\n",
            num_bbs);

        printf("Number of basic blocks with watched memory operations: %lu\n",
            num_bbs_with_watched_ops);

        printf("Number of executed basic blocks: %lu\n",
            num_executed_bbs);

        printf("Number of static memory operations: %lu\n",
            num_static_ops);

        printf("Number of dynamic memory operations: %lu\n",
            num_dynamic_ops);

        printf("Number of watched memory operators: %lu\n",
            num_watched_ops);
    }
}
