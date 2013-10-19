/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.cc
 *
 *  Created on: 2013-10-17
 *      Author: Peter Goodman
 */

#include <atomic>

#include "clients/watchpoints/clients/everything_watched_aug/instrument.h"


using namespace granary;

namespace client {


    std::atomic<unsigned> NUM_AUGMENT_FAULTS = ATOMIC_VAR_INIT(0);
    std::atomic<unsigned> NUM_AUGMENT_FAULTS_MISS = ATOMIC_VAR_INIT(0);
    std::atomic<unsigned> NUM_AUGMENT_FAULTS_DUPLICATES = ATOMIC_VAR_INIT(0);
    std::atomic<unsigned> NUM_NULL_BBS = ATOMIC_VAR_INIT(0);
    std::atomic<unsigned> NUM_NATIVE_FAULTS = ATOMIC_VAR_INIT(0);


    // Report on watchpoints statistics.
    void report(void) throw() {
        printf("Number of faults for augmenting: %u\n",
            NUM_AUGMENT_FAULTS.load());
        printf("Number of faults occurring within a basic block that has already faulted: %u\n",
            NUM_AUGMENT_FAULTS_MISS.load());
        printf("Number of faults occurring in the same spot in the same basic block: %u\n",
            NUM_AUGMENT_FAULTS_DUPLICATES.load());
        printf("Number of un-augmented basic blocks: %u\n",
            NUM_NULL_BBS.load());
        printf("Number of faults because native code accessed watched memory: %u\n",
            NUM_NATIVE_FAULTS.load());
    }

}

