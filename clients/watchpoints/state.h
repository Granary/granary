/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINTS_STATE_H_
#define WATCHPOINTS_STATE_H_

/// Bounds checking watchpoint policy.
#ifdef CLIENT_WATCHPOINT_BOUND
#   include "clients/watchpoints/clients/bounds_checker/state.h"
#endif


/// Leak detector policy.
#ifdef CLIENT_WATCHPOINT_LEAK
#   include "clients/watchpoints/clients/leak_detector/state.h"
#endif

#ifdef CLIENT_SHADOW_MEMORY
#   include "clients/watchpoints/clients/shadow_memory/state.h"
#endif

#endif /* WATCHPOINTS_STATE_H_ */
