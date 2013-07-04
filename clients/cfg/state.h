/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef CFG_STATE_H_
#define CFG_STATE_H_

#define CLIENT_basic_block_state
#define CLIENT_thread_state
#define CLIENT_commit_to_basic_block_state

#include "granary/register.h"
#include "granary/smp/spin_lock.h"

namespace client {


    /// Forward declaration.
    struct basic_block_state;

    enum basic_block_edge_kind {
        BB_EDGE_UNUSED=0,
        BB_EDGE_INTRA_INCOMING,
        BB_EDGE_INTRA_OUTGOING,
        BB_EDGE_INTER_OUTGOING,
        BB_EDGE_INTER_INCOMING
    };

    /// An edge in one of the control-flow graphs.
    struct basic_block_edge {

        /// What kind of edge is this?
        basic_block_edge_kind kind:3;

        /// Records either the source or the sink basic block id, depending
        /// on the edge kind.
        uint16_t block_id:13;

        /// Records either the source or the sink function id, depending on
        /// the edge kind.
        uint16_t function_id;

    } __attribute__((packed));


    /// State that is automatically maintained for each instrumented basic
    /// basic block.
    struct basic_block_state {
    public:

        /// Used to chain all constructed basic blocks together for later
        /// iterating / dumping.
        basic_block_state *next;

        /// Conservative set of live registers on entry to this basic block.
        uint32_t entry_regs;

        /// Set of all used registers read or written within this basic block.
        /// This is helpful for later analyses as a bitmask for all
        uint32_t used_regs;

        /// Number of times this basic block was executed.
        std::atomic<uint16_t> num_executions;

        struct {
            /// Unique ID for this basic block.
            uint16_t block_id:15;

            bool is_app_code:1;

        } __attribute__((packed));

        struct {
            /// Function ID of this basic block. This will be the basic block id of
            /// the entry block for this function.
            ///
            /// Note: This assumes that the function has a single entry-point.
            uint16_t function_id:14;

            bool is_function_entry:1;
            bool is_function_exit:1;

        } __attribute__((packed));

        enum {
            NUM_EDGE_SLOTS = 8
        };

        /// Edges within either of the inter- and intra-procedural control-flow
        /// graph.
        ///
        /// Note: Edges are placed where they will fit, e.g. if all of the edge
        ///       slots in one basic block are full then we will try to fill
        ///       edge slots in the source basic block.
        basic_block_edge edges[NUM_EDGE_SLOTS];

        /// Lock that is acquired when updating the edges of any of the graphs.
        granary::smp::atomic_spin_lock edge_lock;

#if GRANARY_IN_KERNEL
        /// Name of the module being instrumented.
        const char *app_name;

        /// Offset within that module's `.text` section.
        uint32_t app_offset_begin;
        uint16_t num_bytes_in_block;

        /// Number of times this basic block was interrupted.
        std::atomic<uint16_t> num_interrupts;
#endif
    };


    /// State that is automatically maintained for each thread.
    struct thread_state {

        /// Tracking the last inter- and intra-procedural control-flow graph
        /// blocks.
        client::basic_block_state *last_inter;
        client::basic_block_state *last_intra;
    };
}

#endif /* CFG_STATE_H_ */
