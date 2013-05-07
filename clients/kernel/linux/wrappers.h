/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-04-19
 *      Author: pag
 */

#ifndef CLIENT_KERNEL_LINUX_WRAPPERS_H_
#define CLIENT_KERNEL_LINUX_WRAPPERS_H_

/// Bounds checking watchpoint policy.
#ifdef CLIENT_WATCHPOINT_BOUND
#   include "clients/watchpoints/policies/kernel/linux/bound_wrappers.h"
#endif


#endif /* CLIENT_KERNEL_LINUX_WRAPPERS_H_ */
