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
#include "granary/spin_lock.h"

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

        /// Unique ID for this basic block.
        uint16_t block_id;

        /// Function ID of this basic block. This will be the basic block id of
        /// the entry block for this function.
        ///
        /// Note: This assumes that the function has a single entry-point.
        uint16_t function_id;

        /// Edges within either of the inter- and intra-procedural control-flow
        /// graph.
        ///
        /// Note: Edges are placed where they will fit, e.g. if all of the edge
        ///       slots in one basic block are full then we will try to fill
        ///       edge slots in the source basic block.
        ///
        /// TODO: Could technically pack four basic block edges into here if
        ///       we need the space, then turn it into a pointer when more than
        ///       four are needed.
        basic_block_edge *edges;

        struct {
            /// Number of edges allocated for this basic block.
            uint16_t num_edges;

            /// Number of outgoing JMPs in this basic block. This is used in a later
            /// analysis to gage whether or not we've actually gone through all
            /// control-flow paths out of this basic block.
            uint8_t num_outgoing_jumps;

            uint64_t:41; // TODO: unused.

            /// Is this the root node in the inter-procedural control-flow
            /// graph?
            bool is_root:1;

            /// Distinguishes app (e.g. module) from host (e.g. kernel) code.
            bool is_app_code:1;

            /// True iff the instrumented basic block is the entrypoint to a
            /// memory allocator.
            bool is_allocator:1;

            /// True iff the instrumented basic block is the entrypoint to a
            /// memory deallocator.
            bool is_deallocator:1;

            // Note: If both `is_allocator` and `is_deallocator` are true, then
            //       this is a memory re-allocator, e.g. `realloc`.

            /// True iff this basic block ends in something like an indirect
            /// JMP because then we'll (later) assume that not much can be said
            /// about the liveness of registers in successor basic blocks.
            bool has_outgoing_indirect_jmp:1;

            /// Is this basic block an entrypoint to a function? This won't
            /// necessarily be true for all true entrypoints in the sense of
            /// tail-calls.
            bool is_function_entry:1;

            /// Does this basic block end in a RET?
            bool is_function_exit:1;

        } __attribute__((packed));


        /// Lock that is acquired when updating the edges of any of the graphs.
        granary::atomic_spin_lock edge_lock;

#if CONFIG_ENV_KERNEL
        /// Name of the module being instrumented.
        const char *app_name;

        /// Offset within that module's `.text` section.
        uint32_t app_offset_begin;
        uint16_t num_bytes_in_block;

        /// Number of times this basic block was interrupted.
        /// TODO: Can be shorted to uint16_t if we need more meta-info!
        std::atomic<uint16_t> num_interrupts;
#endif
    };


    /// State that is automatically maintained for each thread.
    struct thread_state {

        /// Tracking the last executed basic block.
        client::basic_block_state *last_bb;
    };
}

#endif /* CFG_STATE_H_ */
