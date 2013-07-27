/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.cc
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#include "clients/cfg/events.h"

using namespace granary;

namespace client {


    /// Add an edge to a basic block without locking.
    __attribute__((hot))
    static bool add_edge_unlocked(
        basic_block_state *bb_with_slots,
        uint16_t sink_block_id,
        basic_block_edge_kind kind
    ) throw() {
        bool inserted(false);
        for(unsigned i(0); i < bb_with_slots->num_edges; ++i) {

            basic_block_edge &edge(bb_with_slots->edges[i]);

            // Already connected this basic block to its predecessor.
            if(sink_block_id == edge.block_id && kind == edge.kind) {
                inserted = true;
                break;
            }

            // Found a slot.
            if(BB_EDGE_UNUSED == edge.kind) {
                edge.kind = kind;
                edge.block_id = sink_block_id;
                inserted = true;
                break;
            }
        }
        return inserted;
    }

    /// Add an edge to a basic block.
    __attribute__((hot))
    static bool add_edge(
        basic_block_state *bb_with_slots,
        uint16_t sink_block_id,
        basic_block_edge_kind kind
    ) throw() {
        bb_with_slots->edge_lock.acquire();
        bool inserted(add_edge_unlocked(bb_with_slots, sink_block_id, kind));
        bb_with_slots->edge_lock.release();
        return inserted;
    }


    /// Grow the number of edges.
    __attribute__((hot))
    static void grow_and_add_edge(
        basic_block_state *bb_with_slots,
        uint16_t sink_block_id,
        basic_block_edge_kind kind
    ) throw() {
        bb_with_slots->edge_lock.acquire();
        basic_block_edge *old_edges(bb_with_slots->edges);
        const unsigned old_num_edges(bb_with_slots->num_edges);

        bb_with_slots->num_edges *= 2;
        bb_with_slots->edges = allocate_memory<basic_block_edge>(
            bb_with_slots->num_edges);

        memcpy(
            bb_with_slots->edges,
            old_edges,
            sizeof(basic_block_edge) * old_num_edges);

        add_edge_unlocked(bb_with_slots, sink_block_id, kind);
        bb_with_slots->edge_lock.release();

        free_memory<basic_block_edge>(old_edges, old_num_edges);
    }


    /// Invoked when we enter into a basic block targeted by a CALL instruction.
    __attribute__((hot))
    void event_enter_function(basic_block_state *bb) throw() {
        thread_state_handle thread = safe_cpu_access_zone();
        basic_block_state *last_bb(thread->last_bb);
        thread->last_bb = bb;

        bb->num_executions.fetch_add(1);

        // Connect us to our caller.
        if(!last_bb) {
            return;
        }

        bool added(true);
        if(!add_edge(last_bb, bb->block_id, BB_EDGE_INTER_OUTGOING)) {
            if(!add_edge(bb, last_bb->block_id, BB_EDGE_INTER_INCOMING)) {
                added = false;
            }
        }

        if(!added) {
            if(last_bb->num_edges < bb->num_edges) {
                grow_and_add_edge(last_bb, bb->block_id, BB_EDGE_INTER_OUTGOING);
            } else {
                grow_and_add_edge(bb, last_bb->block_id, BB_EDGE_INTER_INCOMING);
            }
        }
    }


    /// Invoked just before a RET instruction.
    __attribute__((hot))
    void event_exit_function(basic_block_state *) throw() {
        thread_state_handle thread = safe_cpu_access_zone();
        thread->last_bb = nullptr;
    }


    /// Invoked immediately after a function call.
    void event_after_function(basic_block_state *bb) throw() {
        thread_state_handle thread = safe_cpu_access_zone();
        thread->last_bb = bb;
    }


    /// Invoked when we enter into a basic block that is not targeted by a CALL
    /// instruction.
    __attribute__((hot))
    void event_enter_basic_block(basic_block_state *bb) throw() {
        thread_state_handle thread = safe_cpu_access_zone();

        // Update the call stack info.
        basic_block_state *last_bb(thread->last_bb);
        thread->last_bb = bb;

        // Update this basic block.
        bb->num_executions.fetch_add(1);

        if(!last_bb) {
            return;
        }

        bb->function_id = last_bb->function_id;

        // Connect us to any other basic blocks.
        bool added(true);
        if(!add_edge(bb, last_bb->block_id, BB_EDGE_INTRA_INCOMING)) {
            if(!add_edge(last_bb, bb->block_id, BB_EDGE_INTRA_OUTGOING)) {
                added = false;
            }
        }

        if(!added) {
            if(bb->num_edges < last_bb->num_edges) {
                grow_and_add_edge(bb, last_bb->block_id, BB_EDGE_INTRA_INCOMING);
            } else {
                grow_and_add_edge(last_bb, bb->block_id, BB_EDGE_INTRA_OUTGOING);
            }
        }
    }
}
