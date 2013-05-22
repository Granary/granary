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
#   if !CONFIG_TRACE_PRINT_LOG

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
#   endif /* CONFIG_TRACE_PRINT_LOG */
#endif


    /// Log a lookup in the code cache.
    void trace_log::log_entry(
        app_pc IF_TRACE(app_addr),
        app_pc IF_TRACE(target_addr),
        trace_log_target_kind IF_TRACE(kind)
    ) throw() {
#if CONFIG_TRACE_EXECUTION
#   if CONFIG_TRACE_PRINT_LOG
        printf("app=%p cache=%p\n", app_addr, target_addr);
        (void) kind;
#   else
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
#   endif /* CONFIG_TRACE_PRINT_LOG */
#endif
    }


#if CONFIG_TRACE_EXECUTION
    app_pc trace_logger(void) throw() {
        static volatile app_pc routine = nullptr;
        if(routine) {
            return routine;
        }

        instruction_list ls;
        instruction in;
        register_manager all_regs;

        all_regs.kill_all();
        all_regs.revive(reg::arg1);
        all_regs.revive(reg::arg2);

        ls.append(push_(reg::arg1));
        ls.append(push_(reg::arg2));
        ls.append(mov_ld_(reg::arg1, reg::rsp[24]));
        ls.append(mov_ld_(reg::arg2, reg::rsp[16]));

        insert_save_flags_after(ls, ls.last());
        insert_align_stack_after(ls, ls.last());

        IF_USER( in = save_and_restore_xmm_registers(
            all_regs, ls, ls.last(), XMM_SAVE_ALIGNED); )

        in = save_and_restore_registers(all_regs, ls, in);
        in = ls.insert_after(in, mov_imm_(reg::arg3, int64_(TARGET_RUNNING)));
        in = insert_cti_after(ls, in,
            unsafe_cast<app_pc>(trace_log::log_entry),
            true, reg::ret,
            CTI_CALL);

        insert_restore_old_stack_alignment_after(ls, ls.last());
        insert_restore_flags_after(ls, ls.last());
        ls.append(pop_(reg::arg2));
        ls.append(pop_(reg::arg1));
        ls.append(ret_());

        // Encode.
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>( ls.encoded_size()));
        ls.encode(temp);
        BARRIER;
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
        instruction in(ls.prepend(label_()));

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])) );

        in = ls.insert_after(in, push_(reg::rax));
        in = ls.insert_after(in, mov_imm_(
            reg::rax, int64_(reinterpret_cast<uintptr_t>(start_pc))));
        in = ls.insert_after(in, push_(reg::rax));
        in = insert_cti_after(ls, in,
            trace_logger(),
            true, reg::rax,
            CTI_CALL);
        in.set_mangled();
        in = ls.insert_after(in, lea_(reg::rsp, reg::rsp[8]));
        in = ls.insert_after(in, pop_(reg::rax));

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[REDZONE_SIZE])) );
#endif
    }

}


