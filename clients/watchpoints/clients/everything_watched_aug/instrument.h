/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * watched_policy.h
 *
 *  Created on: 2013-10-17
 *      Author: Peter Goodman
 */

#ifndef WATCHED_AUG_WATCHED_POLICY_H_
#define WATCHED_AUG_WATCHED_POLICY_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::watchpoint_null_policy())
#endif

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
namespace client {

    namespace wp {
        struct watched_policy {

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


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle cpu,
                granary::thread_state_handle thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
#endif
        };
    }

    struct watchpoint_watched_policy
        : public client::watchpoints<wp::watched_policy, wp::watched_policy>
    { };


    struct watchpoint_null_policy : public granary::instrumentation_policy {
    public:


        enum {
            AUTO_INSTRUMENT_HOST = false
        };


        /// Instrument a basic block.
        granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &ls
        ) throw();


        /// Instrument a basic block.
        granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &
        ) throw();


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in module code. Returns true iff the client
        /// handles the interrupt.
        granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw();
#endif

    };


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
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
#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */


#endif /* WATCHED_AUG_WATCHED_POLICY_H_ */
