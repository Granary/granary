/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * null_policy.h
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#ifndef WATCHPOINT_NULL_POLICY_H_
#define WATCHPOINT_NULL_POLICY_H_

#include "clients/watchpoints/instrument.h"

#define GRANARY_INIT_POLICY (client::watchpoint_null_policy())

namespace client {

    namespace wp {
        struct null_policy {

            static void visit_read(
                granary::cpu_state_handle &cpu,
                granary::thread_state_handle &thread,
                granary::basic_block_state &bb,
                granary::instruction_list &ls,
                watchpoint_tracker &tracker,
                unsigned i
            ) throw();


            static void visit_write(
                granary::cpu_state_handle &cpu,
                granary::thread_state_handle &thread,
                granary::basic_block_state &bb,
                granary::instruction_list &ls,
                watchpoint_tracker &tracker,
                unsigned i
            ) throw();


#if CONFIG_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle &cpu,
                granary::thread_state_handle &thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
#endif
        };
    }

    struct watchpoint_null_policy
        : public client::watchpoints<wp::null_policy>
    { };


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


#endif /* WATCHPOINT_NULL_POLICY_H_ */
