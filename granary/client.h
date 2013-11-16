/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * client.h
 *
 *  Created on: 2013-02-05
 *      Author: pag
 */

#ifndef GRANARY_CLIENT_H_
#define GRANARY_CLIENT_H_

#include "granary/globals.h"

#ifndef IF_CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
#   if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
#       define IF_CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT(...) __VA_ARGS__
#   else
#       define IF_CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT(...)
#   endif
#endif

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include "granary/state.h"
#   include "granary/instruction.h"
#   include "granary/policy.h"
#   include "granary/detach.h"
#   include "granary/emit_utils.h"
#   include "granary/register.h"
#   include "granary/printf.h"
#   include "granary/dynamorio.h"
#   include "granary/code_cache.h"
#   include "granary/basic_block.h"
#   define DECLARE_POLICY(policy_name, auto_instrument_host) \
        struct policy_name : public granary::instrumentation_policy { \
        public: \
            \
            enum { \
                AUTO_INSTRUMENT_HOST = auto_instrument_host \
            }; \
            \
            granary::instrumentation_policy visit_app_instructions( \
                granary::cpu_state_handle, \
                granary::basic_block_state &, \
                granary::instruction_list &ls \
            ) throw(); \
            \
            granary::instrumentation_policy visit_host_instructions( \
                granary::cpu_state_handle, \
                granary::basic_block_state &, \
                granary::instruction_list & \
            ) throw(); \
            \
            IF_CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT( \
            granary::interrupt_handled_state handle_interrupt( \
                granary::cpu_state_handle, \
                granary::thread_state_handle, \
                granary::basic_block_state &, \
                granary::interrupt_stack_frame &, \
                granary::interrupt_vector \
            ) throw(); ) \
        }

    namespace client {
#   if CONFIG_FEATURE_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in kernel code.
        granary::interrupt_handled_state handle_kernel_interrupt(
            granary::cpu_state_handle cpu,
            granary::thread_state_handle thread,
            granary::interrupt_stack_frame &isf,
            granary::interrupt_vector vector
        ) throw();
#   endif
    }
#else
#   define DECLARE_POLICY(policy_name, auto_instrument_host)
#endif

#endif /* GRANARY_CLIENT_H_ */
