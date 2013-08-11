/*
 * instrument.h
 *
 *  Created on: 2013-08-09
 *      Author: akshayk
 */

#ifndef _RCU_INSTRUMENT_H_
#define _RCU_INSTRUMENT_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::watchpoint_rcu_policy())
#endif


namespace client {


    namespace wp {
        struct rcu_policy {

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

    }


    /// Base policy for the selective shadowing. This makes sure that for all memory
    /// reads/writes to watched objects, the corresponding shadow bit gets updated.
    struct watchpoint_rcu_policy
        : public client::watchpoints<wp::rcu_policy, wp::rcu_policy>
    { };


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





#endif /* INSTRUMENT_H_ */
