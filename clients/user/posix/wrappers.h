/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-04-19
 *      Author: pag
 */

#ifndef CLIENT_USER_POSIX_WRAPPERS_H_
#define CLIENT_USER_POSIX_WRAPPERS_H_



#if defined(CLIENT_WATCHPOINT_BOUND) \
 || defined(CLIENT_WATCHPOINT_WATCHED) \
 || defined(CLIENT_WATCHPOINT_STATS)
#   include "clients/watchpoints/kernel/linux/wrappers.h"
#endif



#endif /* CLIENT_USER_POSIX_WRAPPERS_H_ */
