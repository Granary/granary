/*
 * leak_policy.h
 *
 *  Created on: 2013-06-21
 *      Author: akshayk, pgoodman
 */

#ifndef WATCHPOINT_LEAK_POLICY_H_
#define WATCHPOINT_LEAK_POLICY_H_


#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::leak_policy_enter())
#endif

namespace client {


    namespace wp {
        struct leak_policy {

            enum {
                AUTO_INSTRUMENT_HOST = false
            };

            static void visit_read(
                granary::basic_block_state &bb,
                granary::instruction_list &ls,
                watchpoint_tracker &tracker,
                unsigned i
            ) throw();


            static void visit_write(
                granary::basic_block_state &bb,
                granary::instruction_list &ls,
                watchpoint_tracker &tracker,
                unsigned i
            ) throw();


#if CONFIG_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle cpu,
                granary::thread_state_handle thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
#endif
        };

      //  bool is_active_watchpoint(void*);
    }


    /// Base policy for the leak detector. This makes sure that all memory
    /// reads/writes to watched objects mark those objects as accessed.
    struct watchpoint_leak_policy
        : public client::watchpoints<wp::leak_policy, wp::leak_policy>
    { };


    /// Policy that applies only to the first basic blocks of app code that are
    /// entry points.
    struct leak_policy_enter : public watchpoint_leak_policy {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };


    /// Policy that applies to all non-entry basic blocks of only the first
    /// entered app code function.
    struct leak_policy_exit : public watchpoint_leak_policy {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };


    /// Policy that applies to all internal app code, i.e. invoked from either
    /// an entry block or a potential exit block.
    struct leak_policy_continue : public watchpoint_leak_policy {
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();
    };


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw();
#endif

}

#endif /* WATCHPOINT_BOUND_POLICY_H_ */
