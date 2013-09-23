/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * descriptor.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_DESCRIPTOR_H_
#define RCUDBG_DESCRIPTOR_H_

namespace client {


    struct rcu_dereference_tag { };
    struct rcu_assign_pointer_tag { };


    /// Counter index for an `rcudbg` watched address.
    union rcudbg_counter_index {

        struct {
            uint16_t assign_location_id:14;
            uint16_t:2;
        } __attribute__((packed));

        struct {
            uint16_t read_section_id:14;

            /// Was this an assigned pointer (using `rcu_assign_pointer`) or a
            /// dereferenced pointer (using `rcu_dereference`).
            bool is_deref:1;

            uint16_t:1; // high, shifted out.
        } __attribute__((packed));

        uint16_t as_uint;

    } __attribute__((packed));


    static_assert(sizeof(uint16_t) == sizeof(rcudbg_counter_index),
        "Invalid packing of `rcudbg_counter_index` structure.");


    /// Structure of a watched address.
    union rcudbg_watched_address {

        struct {
            uint64_t:49;

            /// ID for the pointer assign location. Overlaps with
            /// `read_section_id`.
            uint64_t assign_location_id:14;

            uint64_t:1;
        } __attribute__((packed));

        struct {
            uint64_t:3;                  // low

            /// Offset into shadow memory for marking certain pointer bits as
            /// non-dereferenceable without an `rcu_dereference`.
            uint64_t shadow_offset:16;

            uint64_t:29;

            bool IF_USER_ELSE(is_watched, is_unwatched):1;

            /// ID for the read-side critical section. Overlaps with
            /// `assign_location_id`.
            uint64_t read_section_id:14;

            /// Was this an assigned pointer (using `rcu_assign_pointer`) or a
            /// dereferenced pointer (using `rcu_dereference`).
            bool is_deref:1;            // high
        } __attribute__((packed));

        uintptr_t as_uint;
        void *as_pointer;

    } __attribute__((packed));


    static_assert(sizeof(uint64_t) == sizeof(rcudbg_watched_address),
        "Invalid packing of `rcudbg_watched_address` structure.");


    /// Descriptor type for RCU debugging. Note: this object is never allocated!
    struct rcudbg_descriptor {
    public:

        /// Initialisation for an `rcu_dereference`d pointer.
        static bool allocate_and_init(
            rcudbg_descriptor *, // descriptor, unused
            uintptr_t &counter_index,
            uintptr_t, // inherited index, unused
            rcudbg_watched_address deref_address,
            rcudbg_watched_address derefed_address,
            const char *carat,
            rcu_dereference_tag
        ) throw();


        /// Initialisation for an `rcu_assign_pointer`d pointer.
        static bool allocate_and_init(
            rcudbg_descriptor *, // descriptor, unused
            uintptr_t &counter_index,
            uintptr_t, // inherited index, unused
            rcudbg_watched_address assign_pointer,
            rcudbg_watched_address assigned_pointer,
            const char *carat,
            rcu_assign_pointer_tag
        ) throw();

        /// Unused by `rcudbg`.
        static void free(rcudbg_descriptor *, uintptr_t) throw() { }
        static void assign(rcudbg_descriptor *, uintptr_t) throw() { }
        static rcudbg_descriptor *access(uintptr_t) throw() { return nullptr; }
    };


    /// Specify the descriptor type to the watchpoints instrumentation.
    namespace wp {

        template <typename>
        struct descriptor_type {
            typedef rcudbg_descriptor type;
            enum {
                ALLOC_AND_INIT = true,
                REINIT_WATCHED_POINTERS = true
            };
        };
    }
}


#endif /* RCUDBG_DESCRIPTOR_H_ */
