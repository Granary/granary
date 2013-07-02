/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * interrupts.cc
 *
 *  Created on: 2013-06-13
 *      Author: Peter Goodman
 */

#include "clients/instrument.h"

#if CONFIG_CLIENT_HANDLE_INTERRUPT

using namespace granary;

namespace client {

    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    interrupt_handled_state handle_kernel_interrupt(
        cpu_state_handle cpu,
        thread_state_handle,
        interrupt_stack_frame &isf,
        interrupt_vector vector
    ) throw() {
        if(VECTOR_GENERAL_PROTECTION == vector && !isf.error_code) {

            instrumentation_policy policy(START_POLICY);
            policy.in_host_context(true);
            policy.force_attach(true);

            mangled_address target(isf.instruction_pointer, policy);
            app_pc translated_target(nullptr);
            if(!cpu->code_cache.load(target.as_address, translated_target)) {
                translated_target = code_cache::find(cpu, target);
            }

            isf.instruction_pointer = translated_target;
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }
}

#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */
