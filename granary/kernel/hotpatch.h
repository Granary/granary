/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * hotpatch.h
 *
 *  Created on: 2013-06-27
 *      Author: Peter Goodman
 */

#ifndef HOTPATCH_H_
#define HOTPATCH_H_

namespace granary {


    /// Given a function at address `addr` that occupies no more than `len`
    /// contiguous bytes, copy the instructions directly from `addr` into a
    /// new buffer of the same size, re-relativize those instructions, and
    /// return a pointer to the new instructions.
    app_pc copy_and_rerelativize_function(const app_pc addr, int len) ;


    /// Prepare to redirect a function.
    void prepare_redirect_function(app_pc old_address) ;


    /// Hot-patch a function at address `old_address` to JMP to `new_address`.
    void redirect_function(app_pc old_address, app_pc new_address) ;

}


#endif /* HOTPATCH_H_ */
