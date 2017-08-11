/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * trace_log.cc
 *
 *  Created on: 2013-05-06
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/trace_log.h"

#if CONFIG_DEBUG_TRACE_EXECUTION
#   include "granary/instruction.h"
#   include "granary/emit_utils.h"
#   include "granary/state.h"
#endif


namespace granary {


#if CONFIG_DEBUG_TRACE_EXECUTION
#   if !CONFIG_DEBUG_TRACE_PRINT_LOG

    /// An item in the trace log.
    struct trace_log_item {

        /// Previous log item in this thread.
        trace_log_item *prev;

        /// A code cache address into a basic block.
        void *code_cache_addr;

#       if CONFIG_DEBUG_TRACE_RECORD_REGS
        /// State of the general purpose registers on entry to this basic block.
        simple_machine_state state;
#       endif /* CONFIG_DEBUG_TRACE_RECORD_REGS */
    };


    /// Head of the log.
    static std::atomic<trace_log_item *> TRACE = ATOMIC_VAR_INIT(nullptr);
    static std::atomic<uint64_t> NUM_TRACE_ENTRIES = ATOMIC_VAR_INIT(0);


    /// A ring buffer representing the trace log.
    static trace_log_item LOGS[CONFIG_DEBUG_NUM_TRACE_LOG_ENTRIES];
#   endif /* CONFIG_DEBUG_TRACE_PRINT_LOG */
#endif /* CONFIG_DEBUG_TRACE_EXECUTION */


    /// Log a lookup in the code cache.
    void trace_log::add_entry(
        app_pc IF_TRACE(code_cache_addr),
        simple_machine_state *IF_TRACE(state)
    ) {
#if CONFIG_DEBUG_TRACE_EXECUTION
#   if CONFIG_DEBUG_TRACE_PRINT_LOG
        printf("app=%p cache=%p\n", app_addr, target_addr);
        (void) kind;
#   else
        trace_log_item *prev(nullptr);
        trace_log_item *item(&(LOGS[
            NUM_TRACE_ENTRIES.fetch_add(1) % CONFIG_DEBUG_NUM_TRACE_LOG_ENTRIES]));
        item->code_cache_addr = code_cache_addr;

#       if CONFIG_DEBUG_TRACE_RECORD_REGS

        // Have to use our internal `memcpy`, because even when telling the
        // compiler not to use xmm registers, it will sometimes optimize an
        // assignment of `item->state = *state` into a libc `memcpy`.
        memcpy(&(item->state), state, sizeof *state);
        item->state.rsp.value_64 += IF_USER_ELSE(REDZONE_SIZE + 8, 8);

#       else
        (void) state;
#       endif /* CONFIG_DEBUG_TRACE_RECORD_REGS */

        do {
            prev = TRACE.load();
            item->prev = prev;
        } while(!TRACE.compare_exchange_weak(prev, item));
#   endif /* CONFIG_DEBUG_TRACE_PRINT_LOG */
#endif
    }

    extern "C" void **kernel_get_cpu_state(void *ptr[]);


#if CONFIG_DEBUG_TRACE_EXECUTION
    static app_pc trace_logger(void) {
        static volatile app_pc routine(nullptr);
        if(routine) {
            return routine;
        }

        instruction_list ls;
        instruction in;
        instruction xmm_tail;
        register_manager all_regs;

        all_regs.kill_all();

        ls.append(push_(reg::rsp));

        in = save_and_restore_registers(all_regs, ls, ls.last());
        in = ls.insert_after(in,
            mov_ld_(reg::arg1, reg::rsp[sizeof(simple_machine_state)]));

#   if CONFIG_DEBUG_TRACE_RECORD_REGS
        in = ls.insert_after(in, mov_st_(reg::arg2, reg::rsp));
#   endif /* CONFIG_DEBUG_TRACE_RECORD_REGS */

        in = insert_save_flags_after(ls, in);
        in = insert_align_stack_after(ls, in);

        xmm_tail = ls.insert_after(in, label_());

#   if 0 && !CONFIG_ENV_KERNEL
        // In some user space programs (e.g. kcachegrind), we need to
        // unconditionally save all XMM registers.
        in = save_and_restore_xmm_registers(
            all_regs, ls, in, XMM_SAVE_ALIGNED);
#   endif /* CONFIG_ENV_KERNEL */

        in = insert_cti_after(ls, in,
            unsafe_cast<app_pc>(trace_log::add_entry),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL);

        xmm_tail = insert_restore_old_stack_alignment_after(ls, xmm_tail);
        xmm_tail = insert_restore_flags_after(ls, xmm_tail);

        ls.append(lea_(reg::rsp, reg::rsp[8]));
        ls.append(ret_());

        // Encode.
        const unsigned size(ls.encoded_size());
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ls.encode(temp, size);

        BARRIER;

        routine = temp;
        return temp;
    }


    static void add_trace_log_call(
        instruction_list &ls,
        instruction in
    ) {
        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )

        in = insert_cti_after(ls, in,
            trace_logger(),
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_CALL);

        in.set_mangled();

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
    }
#endif /* CONFIG_DEBUG_TRACE_EXECUTION */


    /// Log the run of some code. This will add a lot of instructions to the
    /// beginning of an instruction list, as well as replace RET instructions
    /// with tail-calls to the trace logger so that we can observe ourselves
    /// re-entering a given basic block after a CALL.
    void trace_log::log_execution(instruction_list &IF_TRACE(ls)) {
#if CONFIG_DEBUG_TRACE_EXECUTION

        // The first instruction will be a label.
        instruction in(ls.first());

        ASSERT(in.is_valid());

        instruction in_next(in.next());
        add_trace_log_call(ls, in);

        UNUSED(in_next);

        // TODO: It's a bit weird that this works in user space but not kernel
        //       space!!
#   if 0 && !CONFIG_ENV_KERNEL
        for(in = in_next; in.is_valid(); in = in_next) {
            in_next = in.next();
            if(in.is_return()) {

                // We don't need to guard against the redzone for RETs
                // because they're popping their return address off of the
                // stack.
                insert_cti_after(ls, in,
                    trace_logger(),
                    CTI_DONT_STEAL_REGISTER, operand(),
                    CTI_JMP).set_mangled();

                ls.remove(in);
            }
        }
#   endif
#endif
    }

}


