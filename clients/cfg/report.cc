/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.cc
 *
 *  Created on: 2013-06-29
 *      Author: Peter Goodman
 */

#include "granary/client.h"


extern "C" {
    extern int sprintf(char *buf, const char *fmt, ...);
}


namespace client {

    /// Used to link together all basic blocks.
    extern std::atomic<basic_block_state *> BASIC_BLOCKS;


    /// Buffer used to serialise an individual basic block.
    static char BUFFER[1024];


    /// Serialise a basic block into a string.
    static int serialise_basic_block(
        basic_block_state *bb,
        char *buffer
    ) throw() {
        int b(0);

        // Incoming edges
        for(unsigned i(0); i < basic_block_state::NUM_INCOMING_EDGES; ++i) {
            if(!bb->local_incoming[i]) {
                break;
            }
            b += sprintf(&(buffer[b]), "%u -> %u;\n",
                bb->local_incoming[i]->app_offset_begin,
                bb->app_offset_begin);
        }

        // Number of executions
        b += sprintf(&(buffer[b]), "%u [%u] ",
            bb->app_offset_begin,
            bb->num_executions.load());

        // Meta info.
        b += sprintf(&(buffer[b]),
            "// is_entry=%d is_exit=%d func_id=%u used_regs=%u entry_regs=%u",
            bb->is_function_entry,
            bb->is_function_exit,
            bb->function_id,
            bb->used_regs.encode(),
            bb->entry_regs.encode());

#if GRANARY_IN_KERNEL
        // Kernel-specific meta info.
        b += sprintf(&(buffer[b]), " app=%s end=%u interrupts=%u",
            bb->app_name,
            bb->app_offset_end,
            bb->num_interrupts.load());
#endif

        b += sprintf(&(buffer[b]), "\n");
        return b;
    }


    /// Report on all instrumented basic blocks.
    void report(void) throw() {
        basic_block_state *bb(BASIC_BLOCKS.load());

        for(; bb; bb = bb->next) {
            serialise_basic_block(bb, &(BUFFER[0]));
            granary::printf(&(BUFFER[0]));
        }
    }
}
