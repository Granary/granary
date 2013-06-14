/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * null_policy.cc
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#include "clients/watchpoints/policies/null_policy.h"

using namespace granary;

namespace client { namespace wp {

    void null_policy::visit_read(
        granary::basic_block_state &,
        instruction_list &,
        watchpoint_tracker &,
        unsigned
    ) throw() { }


    void null_policy::visit_write(
        granary::basic_block_state &,
        instruction_list &,
        watchpoint_tracker &,
        unsigned
    ) throw() { }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state null_policy::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector
    ) throw() {
        return INTERRUPT_DEFER;
    }
} /* wp namespace */
#else
} /* wp namespace */
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

} /* client namespace */
