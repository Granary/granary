/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * trace_log.cc
 *
 *  Created on: 2013-05-06
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/trace_log.h"

#if CONFIG_TRACE_CODE_CACHE_FIND
#   include "granary/instruction.h"
#   include "granary/emit_utils.h"
#endif

namespace granary {

#if CONFIG_TRACE_CODE_CACHE_FIND
#   define I(...) __VA_ARGS__

    struct trace_log_item {

        /// Previous log item in this thread.
        trace_log_item *prev;

        /// Native address.
        void *app_address;

        /// Code cache address representing the translated version of
        /// the native address.
        void *cache_address;

        /// Kind of log item.
        trace_log_target_kind kind;
    };


    /// Head of the log.
    static std::atomic<trace_log_item *> TRACE = ATOMIC_VAR_INIT(nullptr);


    /// Configuration for log items.
    struct trace_log_allocator_config {
        enum {
            SLAB_SIZE = 1024 * sizeof(trace_log_item),
            EXECUTABLE = false,
            TRANSIENT = false,
            SHARED = true,
            EXEC_WHERE = EXEC_NONE,
            MIN_ALIGN = 1
        };
    };


    /// Allocator for the log.
    static static_data<
        bump_pointer_allocator<trace_log_allocator_config>
    > TRACE_LOG_ALLOCATOR;


    STATIC_INITIALISE_ID(trace_logger, {
        TRACE_LOG_ALLOCATOR.construct();
    })

#else
#   define I(...)
#endif


    /// Log a lookup in the code cache.
    void log_code_cache_find(
        app_pc I(app_addr),
        app_pc I(target_addr),
        trace_log_target_kind I(kind)
    ) throw() {
#if CONFIG_TRACE_CODE_CACHE_FIND
        trace_log_item *prev(nullptr);
        trace_log_item *item(TRACE_LOG_ALLOCATOR->allocate<trace_log_item>());
        item->app_address = app_addr;
        item->cache_address = target_addr;
        item->kind = kind;

        do {
            prev = TRACE.load();
            item->prev = prev;
        } while(!TRACE.compare_exchange_weak(prev, item));
#endif
    }


    /// Log the run of some code. This will add a lot of instructions to the
    /// beginning of an instruction list.
    void log_code_cache_run(instruction_list &I(ls)) throw() {
#if CONFIG_TRACE_CODE_CACHE_FIND
        instruction first(ls.first());
        register_manager all_regs;
        all_regs.kill_all();

        uint64_t first_pc(0);
        for(; !first_pc && first.is_valid(); first = first.next()) {
            first_pc = reinterpret_cast<uint64_t>(first.pc());
        }

        instruction in(ls.prepend(label_()));
        in = save_and_restore_registers(all_regs, ls, in);
        in = insert_save_flags_after(ls, in);
        in = insert_align_stack_after(ls, in);
        in = ls.insert_after(in, mov_imm_(reg::arg1, int64_(first_pc)));

        instruction call_target(label_());
        in = ls.insert_after(in, mangled(call_(instr_(call_target))));
        in = ls.insert_after(in, call_target);
        in = ls.insert_after(in, pop_(reg::arg2));
        in = ls.insert_after(in, mov_imm_(reg::arg3, int64_(TARGET_RUNNING)));

        in = insert_cti_after(ls, in,
            unsafe_cast<app_pc>(log_code_cache_find),
            true, reg::ret,
            CTI_CALL);

        in.set_mangled();

        in = insert_restore_old_stack_alignment_after(ls, in);
        in = insert_restore_flags_after(ls, in);
#endif
    }

}


