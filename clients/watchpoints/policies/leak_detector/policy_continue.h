/*
 * policy_continue.h
 *
 *  Created on: 2013-06-22
 *      Author: akshayk
 */

#ifndef _LEAK_POLICY_CONTINUE_H_
#define _LEAK_POLICY_CONTINUE_H_

#include "clients/watchpoints/instrument.h"
#include "granary/types.h"

namespace client {

    namespace wp {


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB

        /* Policy for leak-detector; gets called when it first enters
        the instrumented code.*/

        struct app_leak_policy_continue{

            enum {
                AUTO_INSTRUMENT_HOST = false
            };

            static void init() throw();

            static  void visit_instructions(
                    granary::basic_block_state &bb,
                    granary::instruction_list &ls
            ) throw();

            static void visit_cti(
                    granary::basic_block_state &bb,
                    granary::instruction_list &ls,
                    granary::instruction &in
            ) throw();

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
        struct host_leak_policy_continue {

            static  void visit_instructions(
                    granary::basic_block_state &bb,
                    granary::instruction_list &ls
            ) throw();

            static void visit_cti(
                    granary::basic_block_state &bb,
                    granary::instruction_list &ls,
                    granary::instruction &in
            ) throw();

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

        };


#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
    }


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
    struct watchpoint_leak_policy_continue
        : public client::watchpoints<
              wp::app_leak_policy_continue,
              wp::host_leak_policy_continue
          >
    { };


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




#endif /* POLICY_CONTINUE_H_ */
