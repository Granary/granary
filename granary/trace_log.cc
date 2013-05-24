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

        /// A code cache address into a basic block.
        void *code_cache_addr;

#       if CONFIG_TRACE_RECORD_REGS
        /// State of the general purpose registers on entry to this basic block.
        simple_machine_state state;
#       endif /* CONFIG_TRACE_RECORD_REGS */
    };


    /// Head of the log.
    static std::atomic<trace_log_item *> TRACE = ATOMIC_VAR_INIT(nullptr);
    static std::atomic<uint64_t> NUM_TRACE_ENTRIES = ATOMIC_VAR_INIT(0);


    /// A ring buffer representing the trace log.
    static trace_log_item LOGS[CONFIG_NUM_TRACE_LOG_ENTRIES];
#   endif /* CONFIG_TRACE_PRINT_LOG */
#endif /* CONFIG_TRACE_EXECUTION */


    /// Log a lookup in the code cache.
    void trace_log::add_entry(
        app_pc IF_TRACE(code_cache_addr),
        simple_machine_state *IF_TRACE(state)
    ) throw() {
#if CONFIG_TRACE_EXECUTION
#   if CONFIG_TRACE_PRINT_LOG
        printf("app=%p cache=%p\n", app_addr, target_addr);
        (void) kind;
#   else
        trace_log_item *prev(nullptr);
        trace_log_item *item(&(LOGS[
            NUM_TRACE_ENTRIES.fetch_add(1) % CONFIG_NUM_TRACE_LOG_ENTRIES]));
        item->code_cache_addr = code_cache_addr;

#       if CONFIG_TRACE_RECORD_REGS
        item->state = *state;
#       else
        (void) state;
#       endif /* CONFIG_TRACE_RECORD_REGS */

        do {
            prev = TRACE.load();
            item->prev = prev;
        } while(!TRACE.compare_exchange_weak(prev, item));
#   endif /* CONFIG_TRACE_PRINT_LOG */
#endif
    }


#if CONFIG_TRACE_EXECUTION
    app_pc trace_logger(bool in_xmm_context) throw() {
        static volatile app_pc routine[2] = {nullptr};
        if(routine[in_xmm_context]) {
            return routine[in_xmm_context];
        }

        instruction_list ls;
        instruction in;
        instruction xmm_tail;
        register_manager all_regs;

        all_regs.kill_all();
        in = save_and_restore_registers(all_regs, ls, ls.last());
        in = ls.insert_after(in,
            mov_ld_(reg::arg1, reg::rsp[sizeof(simple_machine_state)]));

#   if CONFIG_TRACE_RECORD_REGS
        in = ls.insert_after(in, mov_st_(reg::arg2, reg::rsp));
#   endif /* CONFIG_TRACE_RECORD_REGS */

        in = insert_save_flags_after(ls, in);
        in = insert_align_stack_after(ls, in);

        xmm_tail = ls.insert_after(in, label_());

#   if !GRANARY_IN_KERNEL
        if(in_xmm_context) {
            in = save_and_restore_xmm_registers(
                all_regs, ls, in, XMM_SAVE_ALIGNED);
        }
#   endif /* GRANARY_IN_KERNEL */

        in = insert_cti_after(ls, in,
            unsafe_cast<app_pc>(trace_log::add_entry),
            true, reg::ret,
            CTI_CALL);

        xmm_tail = insert_restore_old_stack_alignment_after(ls, xmm_tail);
        xmm_tail = insert_restore_flags_after(ls, xmm_tail);

        ls.append(ret_());

        // Encode.
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>( ls.encoded_size()));
        ls.encode(temp);

        BARRIER;

        routine[in_xmm_context] = temp;
        return temp;
    }
#endif /* CONFIG_TRACE_EXECUTION */


    /// Log the run of some code. This will add a lot of instructions to the
    /// beginning of an instruction list.
    void trace_log::log_execution(
        instruction_list &IF_TRACE(ls),
        instrumentation_policy &IF_TRACE(policy)
    ) throw() {
#if CONFIG_TRACE_EXECUTION
        instruction in;

        in = ls.prepend(label_());
        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])) );

        in = insert_cti_after(ls, in,
            trace_logger(policy.is_in_xmm_context()),
            false, operand(),
            CTI_CALL);

        in.set_mangled();

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[REDZONE_SIZE])) );
#endif
    }

}


