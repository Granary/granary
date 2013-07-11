/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-07-10
 *      Author: Peter Goodman
 */

#ifndef CLIENT_NULL_PLUS_INSTRUMENT_H_
#define CLIENT_NULL_PLUS_INSTRUMENT_H_


#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::null_plus_policy())
#endif

namespace client {

    struct null_plus_policy : public granary::instrumentation_policy {
    public:


        enum {
            AUTO_INSTRUMENT_HOST = true
        };


        /// Instrument a basic block.
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &ls
        ) throw();


        /// Instrument a basic block.
        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &
        ) throw();


#if CONFIG_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in module code. Returns true iff the client
        /// handles the interrupt.
        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw();
#endif

    };


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle cpu,
        granary::thread_state_handle thread,
        granary::interrupt_stack_frame &isf,
        granary::interrupt_vector vector
    ) throw();
#endif

}



#endif /* CLIENT_NULL_PLUS_INSTRUMENT_H_ */
