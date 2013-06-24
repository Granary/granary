/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/policies/leak_detector/instrument.h"

namespace client {

    namespace wp {
        extern void leak_notify_thread_enter_module(void) throw();
        extern void leak_notify_thread_exit_module(void) throw();
    }

    static granary::app_pc entry_func_addr;
    static granary::app_pc exit_func_addr;


    static granary::register_manager entry_func_regs;
    static granary::register_manager exit_func_regs;


    /// Initialise the leak detector client.
    void init(void) throw() {
        using namespace granary;

        entry_func_addr = unsafe_cast<app_pc>(
            &wp::leak_notify_thread_enter_module);
        exit_func_addr = unsafe_cast<app_pc>(
            &wp::leak_notify_thread_exit_module);

        entry_func_regs = find_used_regs_in_func(entry_func_addr);
        exit_func_regs = find_used_regs_in_func(exit_func_addr);
    }


    /// Instrument the first entrypoint into instrumented code.
    static void instrument_entry_to_code_cache(
        granary::instruction_list &ls,
        granary::instruction in
    ) throw() {
        using namespace granary;

        in = ls.insert_before(in, label_());
        in = save_and_restore_registers(entry_func_regs, ls, in);
        in = insert_cti_after(
            ls, in,
            entry_func_addr, true, reg::rax,
            CTI_CALL);
        in.set_mangled();
    }


    /// Instrument a return to native code.
    static void instrument_exit_code_cache(
        granary::instruction_list &ls,
        granary::instruction in
    ) throw() {
        using namespace granary;
        in = ls.insert_before(in, label_());
        in = save_and_restore_registers(exit_func_regs, ls, in);
        in = insert_cti_after(
            ls, in,
            exit_func_addr, true, reg::rax,
            CTI_CALL);
        in.set_mangled();
    }


    /// Entry basic block.
    granary::instrumentation_policy leak_policy_enter::visit_app_instructions(
        granary::cpu_state_handle &cpu,
        granary::thread_state_handle &thread,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        using namespace granary;

        instruction in(ls.first());
        for(; in.is_valid(); in = in.next()) {
            if(in.is_call()) {
                in.set_policy(policy_for<leak_policy_continue>());
                instrument_entry_to_code_cache(ls, in);
            } else if(in.is_return()) {
                instrument_exit_code_cache(ls, in);
            }
        }

        watchpoint_null_policy::visit_app_instructions(cpu, thread, bb, ls);

        return granary::policy_for<leak_policy_exit>();
    }


    /// Basic blocks within the entry function.
    granary::instrumentation_policy leak_policy_exit::visit_app_instructions(
        granary::cpu_state_handle &cpu,
        granary::thread_state_handle &thread,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        using namespace granary;

        instruction in(ls.first());
        for(; in.is_valid(); in = in.next()) {
            if(in.is_call()) {
                in.set_policy(policy_for<leak_policy_continue>());
            } else if(in.is_return()) {
                instrument_exit_code_cache(ls, in);
            }
        }

        watchpoint_null_policy::visit_app_instructions(cpu, thread, bb, ls);
        return granary::policy_for<leak_policy_exit>();
    }


    /// Internal code.
    granary::instrumentation_policy leak_policy_continue::visit_app_instructions(
        granary::cpu_state_handle &cpu,
        granary::thread_state_handle &thread,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        watchpoint_null_policy::visit_app_instructions(cpu, thread, bb, ls);
        return granary::policy_for<leak_policy_continue>();
    }
}

