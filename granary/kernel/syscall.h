/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * syscall.h
 *
 *  Created on: 2013-09-23
 *      Author: Peter Goodman
 */

#ifndef GRANARY_SYSCALL_H_
#define GRANARY_SYSCALL_H_

#include "deps/drk/msr.h"

namespace granary {

    uint64_t create_syscall_entrypoint(uint64_t native_msr_lstar) ;
}

#endif /* GRANARY_SYSCALL_H_ */
