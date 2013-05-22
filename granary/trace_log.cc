/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * trace_log.cc
 *
 *  Created on: 2013-05-06
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/trace_log.h"

#if CONFIG_TRACE_EXECUTION
#   include "granary/instruction.h"
#   include "granary/emit_utils.h"
#   include "granary/state.h"
#endif

namespace granary {

#if CONFIG_TRACE_EXECUTION

    /// An item in the trace log.
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
    static std::atomic<uint64_t> NUM_TRACE_ENTRIES = ATOMIC_VAR_INIT(0);


    /// A ring buffer representing the trace log.
    static trace_log_item LOGS[CONFIG_NUM_TRACE_LOG_ENTRIES];

#endif


    /// Log a lookup in the code cache.
    void trace_log::log_find(
        app_pc IF_TRACE(app_addr),
        app_pc IF_TRACE(target_addr),
        trace_log_target_kind IF_TRACE(kind)
    ) throw() {
#if CONFIG_TRACE_EXECUTION
        trace_log_item *prev(nullptr);
        trace_log_item *item(&(LOGS[
            NUM_TRACE_ENTRIES.fetch_add(1) % CONFIG_NUM_TRACE_LOG_ENTRIES]));
        item->app_address = app_addr;
        item->cache_address = target_addr;
        item->kind = kind;

        do {
            prev = TRACE.load();
            item->prev = prev;
        } while(!TRACE.compare_exchange_weak(prev, item));
#endif
    }


#if CONFIG_TRACE_EXECUTION
    app_pc trace_logger(void) throw() {
        static volatile app_pc routine(nullptr);
        if(routine) {
            return routine;
        }

        instruction_list ls;
        instruction in(ls.prepend(label_()));
        instruction end;
        register_manager all_regs;

        all_regs.kill_all();
        all_regs.revive(reg::arg1);
        all_regs.revive(reg::arg2);

        in = ls.insert_after(in, push_(reg::arg1));
        in = ls.insert_after(in, push_(reg::arg2));
        in = ls.insert_after(in, mov_ld_(reg::arg1, reg::rsp[24]));
        in = ls.insert_after(in, mov_ld_(reg::arg2, reg::rsp[16]));

        in = insert_save_flags_after(ls, in);
        in = insert_align_stack_after(ls, in);
        end = insert_restore_old_stack_alignment_after(ls, in);
        end = insert_restore_flags_after(ls, end);

        end = ls.insert_after(end, pop_(reg::arg2));
        end = ls.insert_after(end, pop_(reg::arg2));
        end = ls.insert_after(end, mangled(ret_()));

        // save/restore regs around the trace log entries.
        in = save_and_restore_registers(all_regs, ls, in);
        IF_USER( in = save_and_restore_xmm_registers(
            all_regs, ls, in, XMM_SAVE_ALIGNED); )

        in = ls.insert_after(in, mov_imm_(reg::arg3, int64_(TARGET_RUNNING)));

        in = insert_cti_after(ls, in,
            unsafe_cast<app_pc>(trace_log::log_find),
            true, reg::ret,
            CTI_CALL);

        in.set_mangled();

        // Encode.
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>( ls.encoded_size()));
        ls.encode(temp);
        routine = temp;
        return temp;
    }
#endif


    /// Log the run of some code. This will add a lot of instructions to the
    /// beginning of an instruction list.
    void trace_log::log_execution(
        instruction_list &IF_TRACE(ls),
        app_pc IF_TRACE(start_pc)
    ) throw() {
#if CONFIG_TRACE_EXECUTION
        instruction in(ls.last());
        register_manager rm;

        for(; in.is_valid(); in = in.prev()) {
            rm.visit(in);
        }

        in = ls.prepend(label_());

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])) );

        dynamorio::reg_id_t spill_reg(rm.get_zombie());
        bool restore_spill_reg(dynamorio::DR_REG_NULL == spill_reg);
        if(restore_spill_reg) {
            spill_reg = dynamorio::DR_REG_RAX;
        }
        operand spill(spill_reg);

        if(restore_spill_reg) {
            in = ls.insert_after(in, push_(spill));
        }

        in = ls.insert_after(in, mov_imm_(
            spill, int64_(reinterpret_cast<uintptr_t>(start_pc))));
        in = ls.insert_after(in, push_(spill));
        in = insert_cti_after(ls, in,
            trace_logger(),
            !restore_spill_reg, spill,
            CTI_CALL);
        in.set_mangled();
        in = ls.insert_after(in, lea_(reg::rsp, reg::rsp[8]));
        if(restore_spill_reg) {
            in = ls.insert_after(in, pop_(spill));
        }

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[REDZONE_SIZE])) );
#endif
    }

}


