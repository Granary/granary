/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-07-18
 *      Author: Peter Goodman
 */

#ifndef CLIENT_WATCHPOING_STATS_INSTRUMENT_H_
#define CLIENT_WATCHPOING_STATS_INSTRUMENT_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::watchpoint_stats_policy())
#endif

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
namespace client {

    namespace wp {
        struct stats_policy {

            enum {
                AUTO_INSTRUMENT_HOST = false
            };

            static void visit_read(
                granary::basic_block_state &bb,
                granary::instruction_list &ls,
                watchpoint_tracker &tracker,
                unsigned i
            ) ;


            static void visit_write(
                granary::basic_block_state &bb,
                granary::instruction_list &ls,
                watchpoint_tracker &tracker,
                unsigned i
            ) ;


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle cpu,
                granary::thread_state_handle thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) ;
#endif
        };
    }


    struct watchpoint_stats_policy
        : public client::watchpoints<wp::stats_policy, wp::stats_policy>
    {
        /// Visit app instructions (module, user program)
        granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &
        ) ;

        /// Visit host instructions (module, user program)
        granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &
        ) ;
    };


#if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) ;
#endif
}
#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */


#endif /* CLIENT_WATCHPOING_STATS_INSTRUMENT_H_ */
