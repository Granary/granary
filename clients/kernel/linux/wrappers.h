/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-04-19
 *      Author: pag
 */

#ifndef CLIENT_KERNEL_LINUX_WRAPPERS_H_
#define CLIENT_KERNEL_LINUX_WRAPPERS_H_

/// Null watchpoint policy.
#ifdef CLIENT_WATCHPOINT_NULL
#   include "clients/watchpoints/policies/kernel/linux/wrappers.h"
#endif


/// Bounds checking and leak detector watchpoint policy wrappers.
#if defined(CLIENT_WATCHPOINT_BOUND) || defined(CLIENT_WATCHPOINT_LEAK)
#   include "clients/watchpoints/policies/kernel/linux/wrappers.h"
#   include "clients/watchpoints/policies/kernel/linux/bound_wrappers.h"
#endif


/// Null policy that taints addresses.
#ifdef CLIENT_WATCHPOINT_WATCHED
#   include "clients/watchpoints/policies/kernel/linux/wrappers.h"
#   include "clients/watchpoints/policies/kernel/linux/watched_wrappers.h"
#endif


#endif /* CLIENT_KERNEL_LINUX_WRAPPERS_H_ */
