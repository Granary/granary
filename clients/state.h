/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */

#ifndef CLIENT_STATE_H_
#define CLIENT_STATE_H_

#if defined(CLIENT_WATCHPOINT_BOUND) \
 || defined(CLIENT_WATCHPOINT_LEAK) \
 || defined(CLIENT_SHADOW_MEMORY) \
 || defined(CLIENT_RCUDBG) \
 || defined(CLIENT_WATCHPOINT_WATCHED) \
 || defined(CLIENT_WATCHPOINT_PROFILE)
#   include "clients/watchpoints/state.h"
#endif


#ifdef CLIENT_CFG
#   include "clients/cfg/state.h"
#endif

#ifdef CLIENT_WATCHPOINT_STATS
#   include "clients/watchpoints/clients/stats/state.h"
#endif

namespace client {

    struct thread_state;
    struct cpu_state;
    struct block_state;


#ifndef CLIENT_thread_state
    /// Extensions to Granary's internal thread-local storage.
    struct thread_state {

    };
#endif /* CLIENT_thread_state */


#ifndef CLIENT_cpu_state
    /// Extensions to Granary's internal CPU-local storage.
    struct cpu_state {
#   if GRANARY_IN_KERNEL

#   endif
    };
#endif /* CLIENT_cpu_state */


#ifndef CLIENT_basic_block_state
    /// Extensions to Granary's internal basic block-local storage.
    struct basic_block_state { };
#endif /* CLIENT_basic_block_state */


#ifndef CLIENT_commit_to_basic_block_state
#   define GR_GRANARY_DEFINES_COMMIT_TO_BB(...) __VA_ARGS__
#   define GR_CLIENT_DEFINES_COMMIT_TO_BB(...)
#else
#   define GR_GRANARY_DEFINES_COMMIT_TO_BB(...)
#   define GR_CLIENT_DEFINES_COMMIT_TO_BB(...) __VA_ARGS__
#endif
    /// Invoked when Granary commits to putting a basic block into the code
    /// cache.
    GR_GRANARY_DEFINES_COMMIT_TO_BB(inline)
    void commit_to_basic_block(basic_block_state &) throw()
    GR_GRANARY_DEFINES_COMMIT_TO_BB({})
    GR_CLIENT_DEFINES_COMMIT_TO_BB(;)


    /// Invoked when Granary detects a race condition and discards a basic
    /// block.
    GR_GRANARY_DEFINES_COMMIT_TO_BB(inline)
    void discard_basic_block(basic_block_state &) throw()
    GR_GRANARY_DEFINES_COMMIT_TO_BB({})
    GR_CLIENT_DEFINES_COMMIT_TO_BB(;)

}

#endif /* CLIENT_STATE_H_ */
