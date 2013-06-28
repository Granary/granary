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


    /// Should this policy auto-instrument host (kernel, libc) code?
    bool instrumentation_policy::AUTO_VISIT_HOST[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ] = {false};


    /// Policy basic block visitor functions for each policy. The code
    /// cache will use this array of function pointers (initialised
    /// partially at compile time and partially at run time) to determine
    /// which client-code basic block visitor functions should be called.
    instrumentation_policy::basic_block_visitor
    instrumentation_policy::APP_VISITORS[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ] = {
        &instrumentation_policy::missing_policy
    };


    instrumentation_policy::basic_block_visitor
    instrumentation_policy::HOST_VISITORS[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ] = {
        &instrumentation_policy::missing_policy
    };


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// Per-policy interrupt handlers. These are invoked when an interrupt
    /// occurs in *non-delayed* instrumented kernel code.
    instrumentation_policy::interrupt_visitor
    instrumentation_policy::INTERRUPT_VISITORS[
        instrumentation_policy::MAX_NUM_POLICY_IDS
    ] = {
        &instrumentation_policy::missing_interrupt
    };
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
        as_uint <<= NUM_MANGLED_BITS;
        as_policy_address.policy_bits = policy_.as_raw_bits;
    }


    /// Extract the original, unmangled address from a mangled address.
    app_pc mangled_address::unmangled_address(void) const throw() {

        /// Used to extract "address space" high-order bits for recovering a
        /// native address.
        union {
            struct {
                uint64_t saved_bits:(64 - NUM_MANGLED_BITS); // low
                unsigned lost_bits:NUM_MANGLED_BITS; // high
            } bits __attribute__((packed));
            const void *as_addr;
            uint64_t as_uint;
        } recovery_addr;

        recovery_addr.as_addr = this;
        recovery_addr.bits.saved_bits = 0ULL;

        return reinterpret_cast<app_pc>(
            (as_uint >> NUM_MANGLED_BITS) | recovery_addr.as_uint);
    }


    /// Instrumentation policy for basic blocks where the policy is missing.
    struct missing_policy_policy : public instrumentation_policy {
    public:


        enum {
            AUTO_INSTRUMENT_HOST = false
        };


        /// Instruction a basic block.
        static instrumentation_policy visit_app_instructions(
            cpu_state_handle &,
            basic_block_state &,
            instruction_list &
        ) throw() {
            return granary::policy_for<missing_policy_policy>();
        }


        /// Instruction a basic block.
        static instrumentation_policy visit_host_instructions(
            cpu_state_handle &,
            basic_block_state &,
            instruction_list &
        ) throw() {
            return granary::policy_for<missing_policy_policy>();
        }

#if CONFIG_CLIENT_HANDLE_INTERRUPT
        /// Handle an interrupt in module code. Returns true iff the client
        /// handles the interrupt.
        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle &,
            granary::thread_state_handle &,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return INTERRUPT_DEFER;
        }
#endif
    };


    /// Function called when a policy is missing (i.e. hasn't been initialised).
    instrumentation_policy instrumentation_policy::missing_policy(
        cpu_state_handle &,
        basic_block_state &,
        instruction_list &
    ) throw() {
        granary_break_on_fault();
        granary_fault();
        return policy_for<missing_policy_policy>();
    }


#if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// A dummy interrupt visitor. This will be invoked if execution
    /// somehow enters into an invalid policy and is interrupted.
    interrupt_handled_state instrumentation_policy::missing_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector
    ) throw() {
        return INTERRUPT_DEFER; // let the kernel handle the interrupt.
    }
#endif


    instrumentation_policy START_POLICY;


    STATIC_INITIALISE_ID(start_policy, {
        START_POLICY = policy_for<decltype(GRANARY_INIT_POLICY)>();
    })
}

