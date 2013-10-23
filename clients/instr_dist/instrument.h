/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-10-22
 *      Author: Peter Goodman
 */

#ifndef CLIENT_INSTR_DIST_INSTRUMENT_H_
#define CLIENT_INSTR_DIST_INSTRUMENT_H_

#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::instr_dist_policy())
#endif

namespace client {

    struct instruction_log {

        enum {
            NUM_RECORDED_INSTANCES = 10
        };

        std::atomic<unsigned> num_instances;
        granary::app_pc instances[NUM_RECORDED_INSTANCES];
    };


    DECLARE_POLICY(instr_dist_policy, false);
}


#endif /* CLIENT_INSTR_DIST_INSTRUMENT_H_ */
