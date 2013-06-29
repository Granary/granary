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

    /// Invoked when we enter into a basic block targeted by a CALL instruction.
    __attribute__((hot))
    void event_enter_function(basic_block_state *bb) throw() {
        thread_state_handle thread;
        thread->last_executed_basic_block = bb;
        ++(bb->num_executions);
    }


    /// Invoked when we enter into a basic block targeted by a CALL instruction.
    __attribute__((hot))
    void event_exit_function(basic_block_state *) throw() {
        thread_state_handle thread;
        thread->last_executed_basic_block = nullptr;
    }


    /// Invoked when we enter into a basic block that is not targeted by a CALL
    /// instruction.
    __attribute__((hot))
    void event_enter_basic_block(basic_block_state *bb) throw() {
        thread_state_handle thread;
        basic_block_state *last_bb = thread->last_executed_basic_block;
        thread->last_executed_basic_block = bb;

        // Propagate the function ID.
        bb->function_id = last_bb->function_id;
        bb->num_executions += 1;
    }


    /// Invoked before we make an indirect call.
    __attribute__((hot))
    void event_call_indirect(basic_block_state *) throw() {

    }


    /// Invoked before we call app code.
    __attribute__((hot))
    void event_call_app(client::basic_block_state *) throw() {

    }


    /// Invoked before we call host code.
    __attribute__((hot))
    void event_call_host(basic_block_state *, app_pc) throw() {

    }


    /// Invoked when we return from a function call.
    __attribute__((hot))
    void event_return_from_call(basic_block_state *bb) throw() {
        thread_state_handle thread;
        thread->last_executed_basic_block = bb;
    }

}
