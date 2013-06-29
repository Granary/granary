/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#ifndef CFG_INSTRUMENT_H_
#define CFG_INSTRUMENT_H_

#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::cfg_entry_policy())
#endif


namespace client {

    /// Used to instrument the first basic block of a function.
    struct cfg_entry_policy : public granary::instrumentation_policy {
    public:

        enum {
            AUTO_INSTRUMENT_HOST = false
        };

        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();


        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle &,
            granary::basic_block_state &,
            granary::instruction_list &
        ) throw();

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle &,
            granary::thread_state_handle &,
            granary::basic_block_state &bb,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw();
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */
    };


    /// Used to instrument all non-entry basic blocks.
    struct cfg_exit_policy : public granary::instrumentation_policy {
    public:

        enum {
            AUTO_INSTRUMENT_HOST = false
        };

        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();

        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle &,
            granary::basic_block_state &,
            granary::instruction_list &
        ) throw();

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle &,
            granary::thread_state_handle &,
            granary::basic_block_state &bb,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw();
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */
    };


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle &,
        granary::thread_state_handle &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw();
#endif
}


#endif /* CFG_INSTRUMENT_H_ */
