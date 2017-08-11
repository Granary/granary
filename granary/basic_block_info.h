/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * basic_block_data.h
 *
 *  Created on: 2013-11-06
 *      Author: Peter Goodman
 */

#ifndef BASIC_BLOCK_INFO_H_
#define BASIC_BLOCK_INFO_H_

#include "granary/globals.h"

namespace granary {


    /// Forward declarations.
    struct basic_block_info;


    /// Commit to storing information about a trace.
    void store_trace_meta_info(const trace_info &trace) ;


    /// Find the basic block info given an address into our code cache of
    /// basic blocks.
    const basic_block_info *find_basic_block_info(app_pc cache_pc) ;


    /// Remove the basic block info for some `cache_pc`. This is only valid
    /// if the associated basic block was the last block added to this
    /// fragment allocator, and if it wasn't added as part of a trace.
    ///
    /// Note: We assume that there is a coarse grained lock that is guarding
    ///       these operations!
    void remove_basic_block_info(app_pc cache_pc) ;

}

#endif /* BASIC_BLOCK_INFO_H_ */
