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


    /// Allocate basic block data. Basic block data includes both basic block
    /// info (meta-data) and client state (client meta-data).
    ///
    /// Pointers are returned by reference through output parameters.
    basic_block_info *allocate_basic_block_info(
        app_pc start_pc
        _IF_KERNEL( unsigned num_state_bytes )
        _IF_KERNEL( uint8_t *&state_bytes )
    ) throw();


    /// Find the basic block info given an address into our code cache of
    /// basic blocks.
    basic_block_info *find_basic_block_info(app_pc cache_pc) throw();


    /// Try to delete a fragment locator, but don't necessarily succeed.
    ///
    /// Note: We assume that there is a coarse grained lock that is guarding
    ///       these operations!
    bool try_remove_basic_block_info(app_pc cache_pc) throw();

}

#endif /* BASIC_BLOCK_INFO_H_ */
