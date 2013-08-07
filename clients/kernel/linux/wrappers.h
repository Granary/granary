/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-04-19
 *      Author: pag
 */

#ifndef CLIENT_KERNEL_LINUX_WRAPPERS_H_
#define CLIENT_KERNEL_LINUX_WRAPPERS_H_

#if defined(CLIENT_WATCHPOINT_NULL) \
 || defined(CLIENT_WATCHPOINT_BOUND) \
 || defined(CLIENT_WATCHPOINT_LEAK) \
 || defined(CLIENT_WATCHPOINT_WATCHED) \
 || defined(CLIENT_WATCHPOINT_STATS) \
 || defined(CLIENT_SHADOW_MEMORY)
#   include "clients/watchpoints/kernel/linux/wrappers.h"
#endif


#endif /* CLIENT_KERNEL_LINUX_WRAPPERS_H_ */
