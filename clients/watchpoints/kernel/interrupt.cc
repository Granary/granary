/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * interrupts.cc
 *
 *  Created on: 2013-06-13
 *      Author: Peter Goodman
 */

#include "clients/instrument.h"

#if !CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
#   error "The watchpoints system requires `CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT`."
#endif

#if CONFIG_ENV_KERNEL && CONFIG_DEBUG_ASSERTIONS
#   include "granary/kernel/linux/module.h"

extern "C" {
    const kernel_module *kernel_get_module(granary::app_pc);
}


DONT_OPTIMISE
void granary_break_on_gp_in_granary(granary::interrupt_stack_frame *isf) throw() {
    USED(isf);
}

#endif

using namespace granary;

namespace client {

    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    __attribute__((hot))
    interrupt_handled_state handle_kernel_interrupt(
        cpu_state_handle cpu,
        thread_state_handle,
        interrupt_stack_frame &isf,
        interrupt_vector vector
    ) throw() {
        if(VECTOR_GENERAL_PROTECTION != vector || isf.error_code) {
            return INTERRUPT_DEFER;
        }

#if CONFIG_DEBUG_ASSERTIONS && CONFIG_ENV_KERNEL
        const kernel_module *module(
            kernel_get_module(isf.instruction_pointer));
        if(module && module->is_granary) {
            granary_break_on_gp_in_granary(&isf);
        }
#endif

        instrumentation_policy policy(START_POLICY);
        policy.in_host_context(true);
        policy.force_attach(true);
        policy.access_user_data(true); // Can't know for sure, so assume it.
        // TODO: Can't know xmm for sure.

        mangled_address target(isf.instruction_pointer, policy);
        app_pc translated_target(nullptr);
        if(!cpu->code_cache.load(target.as_address, translated_target)) {
            IF_PERF( perf::visit_address_lookup_cpu(false); )
            translated_target = code_cache::find(cpu, target);
            cpu->code_cache.store(target.as_address, translated_target);
        } else {
            IF_PERF( perf::visit_address_lookup_cpu(true); )
        }

        isf.instruction_pointer = translated_target;
        return INTERRUPT_IRET;
    }
}

