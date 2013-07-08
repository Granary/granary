/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.cc
 *
 *  Created on: 2013-06-29
 *      Author: Peter Goodman
 */

#include "granary/client.h"


#define DUMP_META_INFO 0


extern "C" {
    extern int sprintf(char *buf, const char *fmt, ...);
}


namespace client {

    /// Used to link together all basic blocks.
    extern std::atomic<basic_block_state *> BASIC_BLOCKS;


    /// Buffer used to serialise an individual basic block.
    static char BUFFER[granary::PAGE_SIZE];


    enum {
        MAX_NUM_EDGES = 128
    };


    /// Copy of edges in memory.
    static basic_block_edge EDGES[MAX_NUM_EDGES];


    /// Serialise a basic block into a string.
    static int serialise_basic_block(
        basic_block_state *bb,
        char *buffer
    ) throw() {
        int b(0);

        // Make a copy of the edges that we want to dump to avoid race
        // conditions where we see an intermediate state of the edge list, or
        // where we get a pointer to it which is later freed.
        bb->edge_lock.acquire();
        const unsigned num_edges(bb->num_edges);
        ASSERT(MAX_NUM_EDGES >= num_edges);
        memcpy(&(EDGES[0]), bb->edges, num_edges * sizeof(basic_block_edge));
        bb->edge_lock.release();

        // Dump the edges.
        for(unsigned i(0); i < num_edges; ++i) {
            basic_block_edge edge(EDGES[i]);
            if(BB_EDGE_UNUSED == edge.kind) {
                break;
            }

            uint16_t source[2];
            uint16_t sink[2];
            const char *kind;
            bool has_function_edge(false);

            // Get the edge sources/sinks, as well as the correct prefix.
            if(BB_EDGE_INTRA_INCOMING == edge.kind) {
                kind = "b";
                source[0] = edge.block_id;
                sink[0] = bb->block_id;
            } else if(BB_EDGE_INTRA_OUTGOING == edge.kind) {
                kind = "b";
                source[0] = bb->block_id;
                sink[0] = edge.block_id;
            } else if(BB_EDGE_INTER_INCOMING == edge.kind) {
                kind = "f";
                source[0] = edge.function_id;
                source[1] = edge.block_id;
                sink[0] = bb->block_id;
                sink[1] = bb->block_id;
                has_function_edge = true;
            } else {
                kind = "f";
                source[0] = bb->block_id;
                source[1] = bb->block_id;
                sink[0] = edge.block_id;
                sink[1] = edge.function_id;
                has_function_edge = true;
            }

            // Block-to-block, or function-to-function.
            b += sprintf(&(buffer[b]), "%s%u -> %s%u;\n",
                kind, source[0], kind, sink[0]);

            // Block-to-block-function.
            if(has_function_edge) {
                b += sprintf(&(buffer[b]), "b%u -> b%u_f%u;\n",
                    source[1], sink[0], sink[1]);

                b += sprintf(&(buffer[b]), "b%u_f%u [label=f%u style=dotted];\n",
                    sink[0], sink[1], sink[1]);
            }
        }

        const char *color(bb->is_app_code ? "" : " color=blue");

        // This is a function entry, so print a function node for it as well.
        if(bb->is_function_entry) {
            b += sprintf(&(buffer[b]), "f%u [shape=square%s];\n",
            bb->block_id,
            color);
        }

        const char *shape(bb->is_function_exit ? " shape=doublecircle" : "");

        // Print the basic block node.
        b += sprintf(&(buffer[b]), "b%u [label=%u%s%s]; ",
            bb->block_id,
            bb->num_executions.load(),
            shape,
            color);

#if DUMP_META_INFO
        // Meta info.
        b += sprintf(&(buffer[b]),
            "// is_entry=%d is_exit=%d is_app=%d num_execs=%u func_id=%u used_regs=%u entry_regs=%u",
            bb->is_function_entry,
            bb->is_function_exit,
            bb->is_app_code,
            bb->num_executions.load(),
            bb->function_id,
            bb->used_regs,
            bb->entry_regs);

#   if GRANARY_IN_KERNEL
        // Kernel-specific meta info.
        b += sprintf(&(buffer[b]), " module=%s begin=%u end=%u interrupts=%u",
            bb->app_name,
            bb->app_offset_begin,
            bb->app_offset_begin + bb->num_bytes_in_block,
            bb->num_interrupts.load());
#   endif /* GRANARY_IN_KERNEL */
#endif /* DUMP_META_INFO */

        b += sprintf(&(buffer[b]), "\n");
        return b;
    }


    /// Report on all instrumented basic blocks.
    void report(void) throw() {
        basic_block_state *bb(BASIC_BLOCKS.load());

        for(; bb; bb = bb->next) {
            ASSERT(bb != bb->next);
            serialise_basic_block(bb, &(BUFFER[0]));
            granary::printf(&(BUFFER[0]));
        }
    }
}
