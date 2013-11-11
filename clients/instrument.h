/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#ifndef GRANARY_INSTRUMENT_H_
#define GRANARY_INSTRUMENT_H_


/// Null policy.
#ifdef CLIENT_NULL
#   include "clients/null/instrument.h"
#endif


/// Even/odd switching policy.
#ifdef CLIENT_EVEN_ODD
#   include "clients/even_odd/instrument.h"
#endif


/// Even/odd switching policy, but emulated with a single policy and a runtime
/// check.
#ifdef CLIENT_SINGLE_EVEN_ODD
#   include "clients/single_even_odd/instrument.h"
#endif



/// Instruction distribution policy
#ifdef CLIENT_INSTR_DIST
#   include "clients/instr_dist/instrument.h"
#endif


/// Null plus (some kernel space) policy.
#ifdef CLIENT_NULL_PLUS
#   include "clients/null_plus/instrument.h"
#endif


/// Control-flow graph building policy.
#ifdef CLIENT_CFG
#   include "clients/cfg/instrument.h"
#endif


/// Entry policy.
#ifdef CLIENT_ENTRY
#   include "clients/track_entry_exit/instrument.h"
#endif


/// Null policy.
#ifdef CLIENT_WATCHPOINT_NULL
#   include "clients/watchpoints/clients/null/instrument.h"
#endif


/// Null-like policy that detects when we're accessing user data.
#ifdef CLIENT_WATCHPOINT_USER
#   include "clients/watchpoints/clients/user/instrument.h"
#endif


/// Stats policy.
#ifdef CLIENT_WATCHPOINT_STATS
#   include "clients/watchpoints/clients/stats/instrument.h"
#endif


/// Policy where every allocation is watched.
#ifdef CLIENT_WATCHPOINT_WATCHED
#   include "clients/watchpoints/clients/everything_watched/instrument.h"
#endif


/// Policy where every allocation is watched, but where the default
/// instrumentation strategy is to instrument using a NULL policy, and then
/// augment basic blocks that fault into a watchpoint policy.
#ifdef CLIENT_WATCHPOINT_AUGMENT
#   include "clients/watchpoints/clients/everything_watched_aug/instrument.h"
#endif


/// Bounds checking watchpoint policy.
#ifdef CLIENT_WATCHPOINT_BOUND
#   include "clients/watchpoints/clients/bounds_checker/instrument.h"
#endif


/// Leak detector watchpoint tool.
#ifdef CLIENT_WATCHPOINT_LEAK
#   include "clients/watchpoints/clients/leak_detector/instrument.h"
#endif


#ifdef CLIENT_WATCHPOINT_PROFILE
#   include "clients/watchpoints/clients/profiler/instrument.h"
#endif


/// Shadow memory watchpoints tool.
#ifdef CLIENT_SHADOW_MEMORY
#   include "clients/watchpoints/clients/shadow_memory/instrument.h"
#endif


/// RCU debugging watchpoint tool.
#ifdef CLIENT_RCUDBG
#   include "clients/watchpoints/clients/rcudbg/instrument.h"
#endif

#ifdef CLIENT_ARK
#	include "clients/watchpoints/clients/ark/instrument.h"
#endif


/// Default to NULL policy if no policy was chosen.
#ifndef GRANARY_INIT_POLICY
#   error "`GRANARY_INIT_POLICY` not defined; did you forget to include a file in `clients/instrument.h`?"
#endif


#endif /* GRANARY_INSTRUMENT_H_ */
