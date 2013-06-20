/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-04-19
 *      Author: pag
 */

#ifndef CLIENT_USER_POSIX_WRAPPERS_H_
#define CLIENT_USER_POSIX_WRAPPERS_H_


/// Bounds checking watchpoint policy.
#ifdef CLIENT_WATCHPOINT_BOUND
#   include "clients/watchpoints/policies/user/posix/bound_wrappers.h"
#   include "clients/watchpoints/policies/user/posix/wrappers.h"
#endif


/// Bounds checking watchpoint policy.
#ifdef CLIENT_WATCHPOINT_WATCHED
#   include "clients/watchpoints/policies/user/posix/watched_wrappers.h"
#   include "clients/watchpoints/policies/user/posix/wrappers.h"
#endif



#endif /* CLIENT_USER_POSIX_WRAPPERS_H_ */
