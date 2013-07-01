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


    /// Add an edge to a basic block.
    bool add_edge(
        basic_block_state *bb_with_slots,
        uint16_t sink_block_id,
        uint16_t sink_func_id,
        basic_block_edge_kind kind
    ) throw() {
        bool inserted(false);
        bb_with_slots->edge_lock.acquire();
        for(unsigned i(0); i < basic_block_state::NUM_EDGE_SLOTS; ++i) {

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
                edge.function_id = sink_func_id;
                inserted = true;
                break;
            }
        }
        bb_with_slots->edge_lock.release();
        return inserted;
    }


    /// Invoked when we enter into a basic block targeted by a CALL instruction.
    __attribute__((hot))
    void event_enter_function(basic_block_state *bb) throw() {
        thread_state_handle thread;

        basic_block_state *last_bb(
            thread->last_executed_basic_block[thread->call_frame_index]);

        thread->last_executed_basic_block[++thread->call_frame_index] = bb;

        bb->num_executions.fetch_add(1);

        // Connect us to our caller.
        if(!last_bb) {
            return;
        }

        if(!add_edge(last_bb, bb->block_id, bb->function_id, BB_EDGE_INTER_OUTGOING)) {
            if(!add_edge(bb, last_bb->block_id, last_bb->function_id, BB_EDGE_INTER_INCOMING)) {
                granary_break_on_fault(); // TODO
            }
        }
    }


    /// Invoked when we enter into a basic block targeted by a CALL instruction.
    __attribute__((hot))
    void event_exit_function(basic_block_state *) throw() {
        thread_state_handle thread;
        --thread->call_frame_index;
    }


    /// Invoked when we enter into a basic block that is not targeted by a CALL
    /// instruction.
    __attribute__((hot))
    void event_enter_basic_block(basic_block_state *bb) throw() {
        thread_state_handle thread;

        // Update the call stack info.
        basic_block_state *&last_bb_(
            thread->last_executed_basic_block[thread->call_frame_index]);
        basic_block_state *last_bb(last_bb_);
        last_bb_ = bb;

        // Update this basic block.
        bb->function_id = last_bb->function_id;
        bb->num_executions.fetch_add(1);

        if(!last_bb) {
            return;
        }

        // Connect us to any other basic blocks.
        if(!add_edge(bb, last_bb->block_id, last_bb->function_id, BB_EDGE_INTRA_INCOMING)) {
            if(!add_edge(last_bb, bb->block_id, bb->function_id, BB_EDGE_INTRA_OUTGOING)) {
                granary_break_on_fault(); // TODO
            }
        }
    }
}
