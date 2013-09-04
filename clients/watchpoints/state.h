/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINTS_STATE_H_
#define WATCHPOINTS_STATE_H_

/// Bounds checking watchpoint tool.
#ifdef CLIENT_WATCHPOINT_BOUND
#   include "clients/watchpoints/clients/bounds_checker/state.h"
#endif


/// Leak detector tool.
#ifdef CLIENT_WATCHPOINT_LEAK
#   include "clients/watchpoints/clients/leak_detector/state.h"
#endif


#ifdef CLIENT_WATCHPOINT_PROFILE
#   include "clients/watchpoints/clients/profiler/state.h"
#endif

/// Shadow memory tool.
#ifdef CLIENT_SHADOW_MEMORY
#   include "clients/watchpoints/clients/shadow_memory/state.h"
#endif


/// RCU debugger tool.
#ifdef CLIENT_RCUDBG
#   include "clients/watchpoints/clients/rcudbg/state.h"
#endif

#endif /* WATCHPOINTS_STATE_H_ */
