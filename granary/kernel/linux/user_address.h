/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * user_address.h
 *
 *  Created on: 2013-11-05
 *      Author: Peter Goodman
 */

#ifndef KERNEL_USER_ADDRESS_H_
#define KERNEL_USER_ADDRESS_H_

namespace granary {

    /// Forward declarations.
    struct instruction_list;


    /// Try to detect if the basic block contains a specific binary instruction
    /// pattern that makes it look like it could contain and exception table
    /// entry.
    bool kernel_code_accesses_user_data(
        instruction_list &ls,
        app_pc start_pc
    ) ;


    /// Try to get a kernel exception table entry for any instruction within
    /// an instruction list.
    ///
    /// Note: This is very Linux-specific!!
    void *kernel_find_exception_metadata(instruction_list &ls) ;
}


#endif /* KERNEL_USER_ADDRESS_H_ */
