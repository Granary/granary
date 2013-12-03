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

            enum : uint64_t {
                FREE_LIST_END = ~static_cast<uint64_t>(0ULL)
            };

            union {
                struct {
                    /// Most objects won't be more than 16 pages big, so an
                    /// m16&16 parameter suffices (as opposed to m32&32).
                    uint32_t lower_bound;
                    uint32_t upper_bound;
                } __attribute__((packed));

                /// Descriptor index of the next-freed object.
                uint64_t next_free_index;

            } __attribute__((packed));

            /// Low 32 bits of the return address of the code that allocated
            /// this watched object.
            uint32_t return_address;

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
            static bool allocate_and_init(
                bound_descriptor *&,
                uintptr_t &,
                const uintptr_t,
                void *base_address,
                size_t size,
                void *return_address
            ) throw();


            /// Notify the bounds policy that the descriptor can be assigned to
            /// the index.
            static void assign(bound_descriptor *desc, uintptr_t index) throw();


            /// Get the assigned descriptor for a given index.
            static bound_descriptor *access(uintptr_t index) throw();

        } __attribute__((packed));


        static_assert(16 == sizeof(bound_descriptor),
            "Bound descriptor type should be 16 bytes.");


        /// Specify the descriptor type to the generic watchpoint framework.
        template <typename>
        struct descriptor_type {
            typedef bound_descriptor type;
            enum {
                ALLOC_AND_INIT = true,
                REINIT_WATCHED_POINTERS = false
            };
        };


#ifdef GRANARY_DONT_INCLUDE_CSTDLIB
    } /* wp namespace */
#else

        void visit_overflow(
            uintptr_t watched_addr,
            granary::app_pc *addr_in_bb,
            unsigned size
        ) throw();


    } /* namespace wp */

    DECLARE_READ_WRITE_POLICY(
        bound_policy /* name */,
        false /* auto-instrument */)

    DECLARE_INSTRUMENTATION_POLICY(
        watchpoint_bound_policy,
        bound_policy /* app read/write policy */,
        bound_policy /* host read/write policy */,
        { /* override declarations */ })

#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
} /* namespace client */

#endif /* WATCHPOINT_BOUND_POLICY_H_ */
