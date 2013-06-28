/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman, akshayk
 */

#include "clients/watchpoints/policies/leak_detector/instrument.h"

extern "C" void leak_policy_scanner_init(const unsigned char*, const unsigned char*);
#define DECLARE_DESCRIPTOR_ACCESSOR(reg) \
    extern void CAT(granary_access_descriptor_, reg)(void);


#define DECLARE_DESCRIPTOR_ACCESSORS(reg, rest) \
    DECLARE_DESCRIPTOR_ACCESSOR(reg) \
    rest


#define DESCRIPTOR_ACCESSOR_PTR(reg) \
    &CAT(granary_access_descriptor_, reg)


#define DESCRIPTOR_ACCESSOR_PTRS(reg, rest) \
    DESCRIPTOR_ACCESSOR_PTR(reg), \
    rest



extern "C" {
    ALL_REGS(DECLARE_DESCRIPTOR_ACCESSORS, DECLARE_DESCRIPTOR_ACCESSOR)
}


namespace client {

    namespace wp {


        /// Register-specific (generated) functions to mark a leak descriptor
        /// as being accessed.
        typedef void (*descriptor_accessor_type)(void);
        static descriptor_accessor_type DESCRIPTOR_ACCESSORS[] = {
            ALL_REGS(DESCRIPTOR_ACCESSOR_PTRS, DESCRIPTOR_ACCESSOR_PTR)
        };


        /// Register-specific (generated) functions to do bounds checking.
        static unsigned REG_TO_INDEX[] = {
            ~0U,    // null
            0,      // rax
            1,      // rcx
            2,      // rdx
            3,      // rbx
            ~0U,    // rsp
            4,      // rbp
            5,      // rsi
            6,      // rdi
            7,      // r8
            8,      // r9
            9,      // r10
            10,     // r11
            11,     // r12
            12,     // r13
            13,     // r14
            14      // r15
        };


        /// Add instrumentation on every read and write that marks the
        /// watchpoint descriptor as having been accessed.
        void leak_policy::visit_read(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
#if 0
            using namespace granary;
            const unsigned reg_index(REG_TO_INDEX[tracker.regs[i].value.reg]);
            instruction call(insert_cti_after(ls, tracker.labels[i],
                unsafe_cast<app_pc>(DESCRIPTOR_ACCESSORS[reg_index]),
                false, operand(),
                CTI_CALL));
            call.set_mangled();
#else
            UNUSED(ls);
            UNUSED(tracker);
            UNUSED(i);
            UNUSED(REG_TO_INDEX);
            UNUSED(DESCRIPTOR_ACCESSORS);
#endif
        }


        /// Mark the descriptor as having been accessed.
        void leak_policy::visit_write(
            granary::basic_block_state &bb,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            visit_read(bb, ls, tracker, i);
        }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
        granary::interrupt_handled_state leak_policy::handle_interrupt(
            granary::cpu_state_handle &,
            granary::thread_state_handle &,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return granary::INTERRUPT_DEFER;
        }
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

        extern void leak_notify_thread_enter_module(void) throw();
        extern void leak_notify_thread_exit_module(void) throw();
        extern void leak_policy_scan_callback(void) throw();
        extern void leak_policy_update_rootset(void) throw();
    }


    /// Addresses of code cache entry/exit functions.
    static granary::app_pc entry_func_addr;
    static granary::app_pc exit_func_addr;

    /// Registers used by the code cache entry/exit functions.
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
        granary::instruction_list &ls
    ) throw() {
        using namespace granary;

        instruction in(ls.first());
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
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        using namespace granary;
        leak_policy_exit::visit_app_instructions(cpu, bb, ls);
        instrument_entry_to_code_cache(ls);
        return granary::policy_for<leak_policy_exit>();
    }


    /// Basic blocks within the entry function.
    granary::instrumentation_policy leak_policy_exit::visit_app_instructions(
        granary::cpu_state_handle &cpu,
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

        watchpoint_leak_policy::visit_app_instructions(cpu, bb, ls);
        return granary::policy_for<leak_policy_exit>();
    }


    /// Internal code.
    granary::instrumentation_policy leak_policy_continue::visit_app_instructions(
        granary::cpu_state_handle &cpu,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        watchpoint_leak_policy::visit_app_instructions(cpu, bb, ls);
        return granary::policy_for<leak_policy_continue>();
    }
}

