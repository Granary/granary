/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.h
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_BOUND_POLICY_H_
#define WATCHPOINT_BOUND_POLICY_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::watchpoint_bound_policy())
#endif

namespace client {

    namespace wp {

        /// Specifies the bounds for the watched object.
        struct bound_descriptor {

            union {
                struct {
                    /// Most objects won't be more than 16 pages big, so an
                    /// m16&16 parameter suffices (as opposed to m32&32).
                    uint16_t lower_bound;
                    uint16_t upper_bound;
                };

                /// Descriptor index of the next-freed object.
                uint32_t next_free_index;
            };

            /// Descriptor index of this descriptor within the descriptor table.
            uint32_t my_index;

            /// Allocate a watchpoint descriptor.
            static bool allocate(
                bound_descriptor *&,
                uintptr_t &,
                const uintptr_t
            ) throw();


            /// Free a watchpoint descriptor.
            static void free(bound_descriptor *, uintptr_t) throw();


            /// Initialise a watchpoint descriptor.
            static void init(
                bound_descriptor *,
                void *base_address,
                size_t size
            ) throw();

        } __attribute__((packed));


        /// Specify the descriptor type to the generic watchpoint framework.
        template <typename>
        struct descriptor_type {
            typedef bound_descriptor type;
        };


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB


        void visit_overflow(
            uintptr_t watched_addr,
            granary::app_pc *addr_in_bb,
            unsigned size
        ) throw();


        /// Policy for bounds-checking allocated memory in app code.
        struct app_bound_policy {

            enum {
                AUTO_INSTRUMENT_HOST = false
            };

            static void visit_read(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();


            static void visit_write(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();


#   if CONFIG_CLIENT_HANDLE_INTERRUPT
            static granary::interrupt_handled_state handle_interrupt(
                granary::cpu_state_handle &cpu,
                granary::thread_state_handle &thread,
                granary::basic_block_state &bb,
                granary::interrupt_stack_frame &isf,
                granary::interrupt_vector vector
            ) throw();
#   endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */
        };


        /// Policy for bounds-checking allocated memory in host code.
        struct host_bound_policy {

            static void visit_read(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();


            static void visit_write(
                granary::basic_block_state &,
                granary::instruction_list &,
                watchpoint_tracker &,
                unsigned
            ) throw();

            static void visit_overflow(
                uintptr_t watched_addr,
                granary::app_pc *addr_in_bb,
                unsigned size
            ) throw();
        };


#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
    }


#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
    struct watchpoint_bound_policy
        : public client::watchpoints<
              wp::app_bound_policy,
              wp::host_bound_policy
          >
    { };


#   if CONFIG_CLIENT_HANDLE_INTERRUPT
    /// Handle an interrupt in kernel code. Returns true iff the client handles
    /// the interrupt.
    granary::interrupt_handled_state handle_kernel_interrupt(
        granary::cpu_state_handle &,
        granary::thread_state_handle &,
        granary::interrupt_stack_frame &,
        granary::interrupt_vector
    ) throw();
#   endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
}

#endif /* WATCHPOINT_BOUND_POLICY_H_ */
