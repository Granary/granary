/*
 * policy_enter.h
 *
 *  Created on: 2013-06-18
 *      Author: akshayk
 */

#ifndef _LEAK_POLICY_ENTER_H_
#define _LEAK_POLICY_ENTER_H_

#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/policies/leak_detector/policy_exit.h"
#include "clients/watchpoints/policies/leak_detector/policy_continue.h"

namespace client {

    namespace wp {

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB

        /* Policy for leak-detector; gets called when it first enters
        the instrumented code.*/

        struct app_leak_policy_enter{

            enum {
                AUTO_INSTRUMENT_HOST = false
            };

            static void init() throw();

            static void visit_read(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw() { }


            static void visit_write(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw() { }


#   if CONFIG_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle &cpu,
                granary::thread_state_handle &thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
#   endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */
        };


        /// Host policy for the leak detector.
        struct host_leak_policy_enter {

            static void visit_read(
                granary::basic_block_state &,
                granary::instruction_list &,
                client::wp::watchpoint_tracker &,
                unsigned
            ) throw() { }


            static void visit_write(
                granary::basic_block_state &,
                granary::instruction_list &,
                client::wp::watchpoint_tracker &,
                unsigned
            ) throw() { }


        };

        void instrument_entry_to_code_cache(
            granary::instruction_list &ls,
            granary::instruction in
        ) throw();

        void instrument_exit_from_code_cache(
            granary::instruction_list &ls,
            granary::instruction in
        ) throw();

#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
    }


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
    struct leak_policy_enter
        : public client::watchpoints<
              wp::app_leak_policy_enter,
              wp::host_leak_policy_enter
          >
    {
        /// Visit app instructions for leak_policy_enter
        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {

            granary::printf("inside policy leak_enter\n");

            granary::instruction in(ls.first());
            wp::instrument_entry_to_code_cache(ls, in);

            for(; in.is_valid(); in = in.next()) {
                if(in.is_call()) {
                    in.set_policy(granary::policy_for<leak_policy_continue>());
                } else if(in.is_jump()) {
                    in.set_policy(granary::policy_for<leak_policy_exit>());
                } else if(in.is_return()){
                    wp::instrument_exit_from_code_cache(ls, in);
                }
            }

            return client::watchpoints<
                    wp::app_leak_policy_enter,
                    wp::host_leak_policy_enter>
                    ::visit_app_instructions(cpu, thread, bb, ls);
        }

        /// Visit Host instructions for leak_policy_enter
        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            return client::watchpoints<
                    wp::app_leak_policy_enter,
                    wp::host_leak_policy_enter>
                    ::visit_app_instructions(cpu, thread, bb, ls);
        }

    };


#   if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle &,
        granary::thread_state_handle &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw();
#   endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
}

#endif /* _LEAK_POLICY_ENTER_H_ */
