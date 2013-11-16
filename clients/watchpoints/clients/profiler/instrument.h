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
#   define GRANARY_INIT_POLICY (client::watchpoint_profiler_policy())
#endif

namespace client {


    namespace wp {

        struct app_policy {

            enum {
                AUTO_INSTRUMENT_HOST = false
            };

            static void visit_read(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();


            static void visit_write(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();


#   if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle cpu,
                granary::thread_state_handle thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
#   endif /* CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */
        };

        struct host_policy {

            enum {
                AUTO_INSTRUMENT_HOST = false
            };

            static void visit_read(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();


            static void visit_write(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();


#   if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle cpu,
                granary::thread_state_handle thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
#   endif /* CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */
        };

    }


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
    struct watchpoint_profiler_policy
        : public client::watchpoints<
              wp::app_policy,
              wp::host_policy
          >
    { };

#   if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw();
#   endif /* CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT */



#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
}

#endif /* WATCHPOINT_BOUND_POLICY_H_ */
