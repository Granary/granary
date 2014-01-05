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

        cpu_state_handle cpu;

        // Decode the original instructions and extract the CTIs.
        app_pc decode_pc(native_pc_start);
        unsigned cti_num(0);
        for(unsigned j(0); j < bb.info->generating_num_instructions; ++j) {

            // Note: Some module code might fault when decoding (e.g. if it was
            //       some of the initialization code). So, we need to recover
            //       from here.
            if(!granary_try_access(decode_pc)) {
                break;
            }

            instruction in(instruction::decode(&decode_pc));


            if(!in.is_cti() || in.is_return()) {
                continue;
            }

            operand target(in.cti_target());
            if(dynamorio::PC_kind != target.kind) {
#if CFG_RECORD_INDIRECT_TARGETS
                const char *format(nullptr);
                if(in.is_call()) {
                    format = "CALL*(%p,%p)\n";
                } else if(in.is_jump()) {
                    format = "JMP*(%p,%p)\n";
                } else {
                    ASSERT(false);
                }

                const indirect_cti &cti(bb.state()->indirect_ctis[cti_num++]);
                for(unsigned k(0); k < cti.num_indirect_targets; ++k) {
                    if(!cti.indirect_targets[k]) {
                        break;
                    }

                    i += sprintf(
                        &(LOG_BUFF[i]),
                        format,
                        in.pc(), cti.indirect_targets[k]);
                }
#endif
            } else if(in.is_call()) {
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

        UNUSED(cti_num);
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
