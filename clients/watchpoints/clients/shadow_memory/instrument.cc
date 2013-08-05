/*
 * instrument.cc
 *
 *  Created on: 2013-08-04
 *      Author: akshayk
 */


#include "clients/watchpoints/clients/shadow_memory/instrument.h"


namespace client {

    namespace wp {

        /// Add instrumentation on every read and write that marks the
        /// shadow bits for the corresponding types .
        void shadow_policy::visit_read(
            granary::basic_block_state &,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            UNUSED(ls);
            UNUSED(tracker);
            UNUSED(i);
        }


        void shadow_policy::visit_write(
            granary::basic_block_state &bb,
            granary::instruction_list &ls,
            watchpoint_tracker &tracker,
            unsigned i
        ) throw() {
            visit_read(bb, ls, tracker, i);
        }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
        granary::interrupt_handled_state shadow_policy::handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return granary::INTERRUPT_DEFER;
        }
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

    }
}
