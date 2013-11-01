/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * policy.cc
 *
 *  Created on: 2013-01-24
 *      Author: pag
 */

#include "granary/globals.h"
#include "granary/policy.h"
#include "granary/mangle.h"

#include "clients/instrument.h"

namespace granary {


#if !CONFIG_INSTRUMENT_HOST
    /// Should this policy auto-instrument host (kernel, libc) code?
    bool instrumentation_policy::AUTO_VISIT_HOST[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ] = {false};
#endif


    /// Policy basic block visitor functions for each policy. The code
    /// cache will use this array of function pointers (initialised
    /// partially at compile time and partially at run time) to determine
    /// which client-code basic block visitor functions should be called.
    instrumentation_policy::basic_block_visitor
    instrumentation_policy::APP_VISITORS[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ];


    instrumentation_policy::basic_block_visitor
    instrumentation_policy::HOST_VISITORS[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ];


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// Per-policy interrupt handlers. These are invoked when an interrupt
    /// occurs in *non-delayed* instrumented kernel code.
    instrumentation_policy::interrupt_visitor
    instrumentation_policy::INTERRUPT_VISITORS[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ];
#endif


    /// Policy ID tracker.
    std::atomic<unsigned> instrumentation_policy::NEXT_POLICY_ID(
        ATOMIC_VAR_INIT(0));


    /// Get the policy for a policy-extended mangled address.
    instrumentation_policy::instrumentation_policy(
        const mangled_address &addr
    ) throw()
        : as_raw_bits(addr.as_policy_address.policy_bits)
    { }


    /// Mangle an address according to a policy.
    mangled_address::mangled_address(
        app_pc addr_,
        const instrumentation_policy policy_
    ) throw() {
        as_address = addr_;
        as_policy_address.policy_bits = policy_.as_raw_bits;
    }


    /// Extract the original, unmangled address from a mangled address.
    app_pc mangled_address::unmangled_address(void) const throw() {
        mangled_address unmangled_addr;
        unmangled_addr.as_uint = as_uint;
        unmangled_addr.as_policy_address.policy_bits = IF_USER_ELSE(0, 0xFFFF);
        return unmangled_addr.as_address;
    }


    /// Instrumentation policy for basic blocks where the policy is missing.
    struct missing_policy_policy : public instrumentation_policy {
    public:


        enum {
            AUTO_INSTRUMENT_HOST = false
        };


        /// Instruction a basic block.
        instrumentation_policy visit_app_instructions(
            cpu_state_handle,
            basic_block_state &,
            instruction_list &
        ) throw() {
            return granary::policy_for<missing_policy_policy>();
        }


        /// Instruction a basic block.
        instrumentation_policy visit_host_instructions(
            cpu_state_handle,
            basic_block_state &,
            instruction_list &
        ) throw() {
            return granary::policy_for<missing_policy_policy>();
        }

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in module code. Returns true iff the client
        /// handles the interrupt.
        granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return INTERRUPT_DEFER;
        }
#endif
    };


    instrumentation_policy START_POLICY;


    STATIC_INITIALISE_ID(start_policy, {
        START_POLICY = policy_for<decltype(GRANARY_INIT_POLICY)>();
    })
}

