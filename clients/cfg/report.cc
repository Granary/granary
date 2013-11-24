/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.cc
 *
 *  Created on: 2013-06-29
 *      Author: Peter Goodman
 */

#include "clients/cfg/config.h"

#include "granary/client.h"

#if CONFIG_ENV_KERNEL
#   include "granary/kernel/linux/module.h"
#endif

#if CFG_RECORD_EXEC_COUNT
#   define IF_REPORT_COUNT(...) __VA_ARGS__
#   define _IF_REPORT_COUNT(...) , __VA_ARGS__
#   if CFG_RECORD_FALL_THROUGH_COUNT
#       define IF_REPORT_JCC_COUNT(...) __VA_ARGS__
#       define _IF_REPORT_JCC_COUNT(...) , __VA_ARGS__
#   else
#       define IF_REPORT_JCC_COUNT(...)
#       define _IF_REPORT_JCC_COUNT(...)
#   endif
#else
#   define IF_REPORT_COUNT(...)
#   define _IF_REPORT_COUNT(...)
#   define IF_REPORT_JCC_COUNT(...)
#   define _IF_REPORT_JCC_COUNT(...)
#endif

using namespace granary;


extern "C" {
    extern int sprintf(char *buf, const char *fmt, ...);
    IF_KERNEL( extern const kernel_module *kernel_get_module(app_pc addr); )
}


extern "C" {
    extern app_pc GRANARY_EXEC_START;
}


namespace client {


    enum {
        LOG_BUFF_SIZE = 2048
    };


    /// Buffer used for logging.
    static char LOG_BUFF[LOG_BUFF_SIZE] = {'\0'};


    /// Used to link together all basic blocks.
    extern std::atomic<basic_block_state *> BASIC_BLOCKS;


    /// Names for the conditional branches.
    const char *JCC_NAMES[] = {
        "jo",
        "jno",
        "jb",
        "jnb",
        "jz",
        "jnz",
        "jbe",
        "jnbe",
        "js",
        "jns",
        "jp",
        "jnp",
        "jl",
        "jnl",
        "jle",
        "jnle"
    };


    /// Log out information about an individual basic block.
    static int report_bb(const basic_block_state *state) throw() {
        const basic_block bb(state->label.translation);
        const app_pc native_pc_start(
            bb.info->generating_pc.unmangled_address());

        IF_KERNEL( const kernel_module *module(
            kernel_get_module(native_pc_start)); )

        int i(0);
        i += sprintf(&(LOG_BUFF[i]),
            "BB(%x,%u,%p,%u,%p" IF_REPORT_COUNT(",%lu") IF_KERNEL(",%x,%s") ")\n",
            bb.cache_pc_start - GRANARY_EXEC_START,
            bb.info->num_bytes,
            bb.info->allocator,
            bb.info->num_bbs_in_trace,
            native_pc_start
            _IF_REPORT_COUNT(state->num_executions)
            _IF_KERNEL(native_pc_start - module->text_begin)
            _IF_KERNEL(module->name));

        // Decode the original instructions and extract the CTIs.
        app_pc decode_pc(native_pc_start);
        for(unsigned j(0); j < bb.info->generating_num_instructions; ++j) {
            instruction in(instruction::decode(&decode_pc));
            if(!in.is_cti() || in.is_return()) {
                continue;
            }

            operand target(in.cti_target());
            if(dynamorio::PC_kind != target.kind) {
                continue;
            }

            if(in.is_call()) {
                i += sprintf(&(LOG_BUFF[i]), "CALL(%p)\n", target.value.pc);
            } else if(in.is_jump()) {
                i += sprintf(&(LOG_BUFF[i]), "JMP(%p)\n", target.value.pc);
            } else {
                i += sprintf(&(LOG_BUFF[i]),
                    "Jcc(%s,%p,%p" IF_REPORT_JCC_COUNT(",%lu") ")\n",
                    JCC_NAMES[in.op_code() - dynamorio::OP_jo],
                    target.value.pc,
                    decode_pc
                    _IF_REPORT_JCC_COUNT(state->num_fall_through_executions));
            }
        }

        LOG_BUFF[i] = '\0';
        return i;
    }


    /// Report on all instrumented basic blocks.
    void report(void) throw() {
        const basic_block_state *bb(BASIC_BLOCKS.load());

        for(; bb; bb = bb->next) {

            IF_KERNEL( const eflags flags(granary_disable_interrupts()); )
            cpu_state_handle cpu;
            granary::enter(cpu);
            int report_len(report_bb(bb));
            IF_KERNEL( granary_store_flags(flags); )

            // Log with interrupts enabled.
            log(&(LOG_BUFF[0]), report_len);
        }
    }
}


#if 0



#if 0

// Record the name and relative offset of the code within the
// module. We can correlate this offset with an `objdump` or do
// static instrumentation of the ELF/KO based on this offset.
const kernel_module *module(kernel_get_module(start_pc));
if(module) {
    const uintptr_t app_begin(
        reinterpret_cast<uintptr_t>(module->text_begin));

    const uintptr_t start_pc_uint(
        reinterpret_cast<uintptr_t>(start_pc));

    bb.app_offset_begin = start_pc_uint - app_begin;
    bb.num_bytes_in_block = reinterpret_cast<uintptr_t>(end_pc) \
                          - start_pc_uint;
    bb.app_name = module->name;
}
#endif

namespace client {

    /// Used to link together all basic blocks.
    extern std::atomic<basic_block_state *> BASIC_BLOCKS;


    enum {
        BUFFER_SIZE = granary::PAGE_SIZE * 2,
        BUFFER_FLUSH_THRESHOLD = BUFFER_SIZE - 256
    };


    /// Buffer used to serialise an individual basic block.
    static char BUFFER[BUFFER_SIZE];


    enum {
        MAX_NUM_EDGES = 1 << 14
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

            // Get the edge sources/sinks, as well as the correct prefix.
            if(BB_EDGE_INTRA_INCOMING == edge.kind) {
                b += sprintf(&(BUFFER[b]),
                    "INTRA(%d,%d)\n", edge.block_id, bb->block_id);

            } else if(BB_EDGE_INTRA_OUTGOING == edge.kind) {
                b += sprintf(&(BUFFER[b]),
                    "INTRA(%d,%d)\n", bb->block_id, edge.block_id);
            } else if(BB_EDGE_INTER_INCOMING == edge.kind) {
                b += sprintf(&(BUFFER[b]),
                    "INTER(%d,%d)\n", edge.block_id, bb->block_id);
            } else {
                b += sprintf(&(BUFFER[b]),
                    "INTER(%d,%d)\n", bb->block_id, edge.block_id);
            }

            if(b >= BUFFER_FLUSH_THRESHOLD) {
                granary::log(&(BUFFER[0]), b);
                b = 0;
            }
        }

        // Meta info.
        b += sprintf(&(buffer[b]),
            "BB(%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%d",
            bb->is_root,
            bb->is_function_entry,
            bb->is_function_exit,
            bb->is_app_code,
            bb->is_allocator,
            bb->is_deallocator,
            bb->num_executions.load(),
            bb->function_id,
            bb->block_id,
            bb->used_regs,
            bb->entry_regs,
            bb->num_outgoing_jumps,
            bb->has_outgoing_indirect_jmp);

#   if CONFIG_ENV_KERNEL
        // Kernel-specific meta info.
        b += sprintf(&(buffer[b]), ",%s,%u,%u,%u",
            bb->app_name,
            bb->app_offset_begin,
            bb->app_offset_begin + bb->num_bytes_in_block,
            bb->num_interrupts.load());
#   endif /* CONFIG_ENV_KERNEL */

        b += sprintf(&(buffer[b]), ")\n");
        return b;
    }


    /// Report on all instrumented basic blocks.
    void report(void) throw() {
        basic_block_state *bb(BASIC_BLOCKS.load());

        const char *format(
            "BB_FORMAT(is_root,is_function_entry,is_function_exit,is_app_code,"
            "is_allocator,is_deallocator,num_executions,function_id,block_id,"
            "used_regs,entry_regs,num_outgoing_jumps,has_outgoing_indirect_jmp"
#if CONFIG_ENV_KERNEL
            ",app_name,app_offset_begin,app_offset_end,num_interrupts"
#endif /* CONFIG_ENV_KERNEL */
            ")\n"
        );

        granary::log(format, strlen(format));
        for(; bb; bb = bb->next) {
            ASSERT(bb != bb->next);
            int len(serialise_basic_block(bb, &(BUFFER[0])));
            granary::log(&(BUFFER[0]), len);
        }
    }
}
#endif
