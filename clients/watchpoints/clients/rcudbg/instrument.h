/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.cc
 *
 *  Created on: 2013-08-15
 *      Author: Peter Goodman
 */

#ifndef _RCUDBG_INSTRUMENT_H_
#define _RCUDBG_INSTRUMENT_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::rcu_null())
#endif

#if !GRANARY_IN_KERNEL
#   error "The RCU debugger only works for Linux kernel code."
#endif

#if !CONFIG_CLIENT_HANDLE_INTERRUPT
#   error "Interrupt handling must be enabled for RCU debugging."
#endif


namespace client {

    enum read_critical_section_kind {
        RCU_READ_SECTION,
        RCU_READ_SECTION_BH,
        RCU_READ_SECTION_SCHED
    };


    namespace wp {

        struct rcu_read_policy {

            enum {
                AUTO_INSTRUMENT_HOST = true
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


            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle cpu,
                granary::thread_state_handle thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
        };

    }

    enum {
        MAX_SECTION_DEPTH = 4
    };


    /// Forward declarations.
    struct rcu_null;
    template <unsigned depth> struct read_critical_section;


    /// Base case: The maximum depth for rcu read-side critical sections.
    template <>
    struct read_critical_section<MAX_SECTION_DEPTH> {

        enum {
            AUTO_INSTRUMENT_HOST = false
        };

        typedef read_critical_section<MAX_SECTION_DEPTH> self_type;

        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &
        ) throw() {
            ASSERT(false);
            return granary::policy_for<self_type>();
        }

        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle,
            granary::basic_block_state &,
            granary::instruction_list &
        ) throw() {
            ASSERT(false);
            return granary::policy_for<self_type>();
        }

        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw() {
            return granary::INTERRUPT_DEFER;
        }
    };


    /// Returns the policy for a given read-side critical section nesting
    /// depth.
    granary::instrumentation_policy policy_for_depth(
        const unsigned depth
    ) throw();


    /// Add CTI-specific instrumentation for an RCU read-side critical section.
    granary::instruction rcu_instrument_cti(
        granary::instrumentation_policy &curr_policy,
        unsigned &curr_depth,
        granary::instruction_list &ls,
        granary::instruction in
    ) throw();


    /// Policy to apply while instrumenting code executing in a read-side for
    /// depth `depth`.
    template <unsigned depth>
    struct read_critical_section
        : public client::watchpoints<wp::rcu_read_policy, wp::rcu_read_policy>
    {

        enum {
            AUTO_INSTRUMENT_HOST = true
        };

        static granary::instrumentation_policy visit_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            using namespace granary;

            unsigned curr_depth(depth);
            instrumentation_policy curr_policy(policy_for_depth(depth));
            curr_policy.force_attach(true);

            for(instruction in(ls.first()); in.is_valid(); ) {
                instruction next_in(in.next());

                if(in.is_cti()) {
                    in = rcu_instrument_cti(curr_policy, curr_depth, ls, in);
                }

                // Chop the instruction list short because we've transitioned
                // from watchpoints to null, and so we don't want to instrument
                // the instructions within the null (chopped off) region using
                // the watchpoints policy.
                if(0 == curr_depth) {
                    ASSERT(next_in.is_valid() && next_in.pc());
                    const app_pc fall_through_pc(next_in.pc());
                    ls.remove_tail_at(next_in);
                    in = ls.append(jmp_(pc_(fall_through_pc)));
                    in.set_policy(curr_policy);
                    break;
                }

                in = next_in;
            }

            // Run the watchpoints instrumentation.
            client::watchpoints<
                wp::rcu_read_policy, wp::rcu_read_policy
            >::visit_app_instructions(cpu, bb, ls);

            return curr_policy;
        }

        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            return visit_instructions(cpu, bb, ls);
        }

        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw() {
            return visit_instructions(cpu, bb, ls);
        }
    };


    /// Policy for code not running in an RCU read-side critical section, nor
    /// operating on any RCU objects (writers).
    struct rcu_null : public granary::instrumentation_policy {

        enum {
            AUTO_INSTRUMENT_HOST = false
        };

        static granary::instrumentation_policy visit_app_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();

        static granary::instrumentation_policy visit_host_instructions(
            granary::cpu_state_handle cpu,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
        ) throw();

        static granary::interrupt_handled_state handle_interrupt(
            granary::cpu_state_handle,
            granary::thread_state_handle,
            granary::basic_block_state &,
            granary::interrupt_stack_frame &,
            granary::interrupt_vector
        ) throw();
    };


    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle,
        granary::thread_state_handle,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw();
}

#endif /* _RCUDBG_INSTRUMENT_H_ */
