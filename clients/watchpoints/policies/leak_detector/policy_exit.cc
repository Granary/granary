/*
 * policy_exit.c
 *
 *  Created on: 2013-06-21
 *      Author: akshayk
 */

#include "clients/watchpoints/policies/leak_detector/policy_exit.h"
#include "clients/watchpoints/policies/leak_detector/policy_enter.h"
#include "clients/watchpoints/policies/leak_detector/policy_continue.h"
//#include "clients/watchpoints/policies/kernel/linux/leak_wrapper.h"

using namespace granary;

namespace client { namespace wp {

   void app_leak_policy_exit::visit_instructions(
            granary::basic_block_state &bb,
            granary::instruction_list &ls) throw() {
        UNUSED(bb);
        UNUSED(ls);
        granary::printf("inside APP exit visit instructions");
    }


    void app_leak_policy_exit::visit_cti(
            granary::basic_block_state &bb,
            granary::instruction_list &ls,
            granary::instruction &in) throw() {
        UNUSED(bb);
        UNUSED(ls);
 	UNUSED(in);
    }

    void app_leak_policy_exit::visit_read(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
    }


    void app_leak_policy_exit::visit_write(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
    }


    void host_leak_policy_exit::visit_instructions(
            granary::basic_block_state &bb,
            granary::instruction_list &ls) throw() {
        UNUSED(bb);
        UNUSED(ls);
    }


    void host_leak_policy_exit::visit_cti(
            granary::basic_block_state &bb,
            granary::instruction_list &ls,
            granary::instruction &in) throw() {
        UNUSED(bb);
        UNUSED(ls);
        UNUSED(in);
    }

    void host_leak_policy_exit::visit_read(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
    }


    void host_leak_policy_exit::visit_write(
        granary::basic_block_state &,
        instruction_list &ls,
        watchpoint_tracker &tracker,
        unsigned i
    ) throw() {
        UNUSED(ls);
        UNUSED(tracker);
        UNUSED(i);
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state app_leak_policy_exit::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }
} /* wp namespace */
#else
} /* wp namespace */
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

} /* client namespace */



