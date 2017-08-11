/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * dbl.h
 *
 *  Created on: 2013-11-13
 *      Author: Peter Goodman
 */

#ifndef GRANARY_DBL_H_
#define GRANARY_DBL_H_

namespace granary {

    /// Replace `cti` with a new instruction that jumps to the DBL entry routine
    /// for instruction patching and replacing.
    void insert_dbl_lookup_stub(
        instruction_list &ls,
        instruction_list &stub_ls,
        instruction cti,
        mangled_address target_address
    ) ;

}

#endif /* GRANARY_DBL_H_ */
