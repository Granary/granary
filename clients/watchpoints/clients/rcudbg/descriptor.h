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

    /// Counter index for an `rcudbg` watched address.
    union rcudbg_counter_index {
        struct {
            union {
                uint64_t read_section_id:14;
                uint64_t assign_location_id:14;
            } __attribute__((packed));

            /// Was this an assigned pointer (using `rcu_assign_pointer`) or a
            /// dereferenced pointer (using `rcu_dereference`).
            bool is_deref:1;

            uint16_t:1; // high, shifted out.
        } __attribute__((packed));

        uint16_t as_uint;
    };


    static_assert(sizeof(uint16_t) == sizeof(rcudbg_counter_index),
        "Invalid packing of `rcudbg_counter_index` structure.");


    /// Structure of a watched address.
    union rcudbg_watched_address {
        struct {
            uint64_t:3;                  // low

            /// Offset into shadow memory for marking certain pointer bits as
            /// non-dereferenceable without an `rcu_dereference`.
            uint64_t shadow_offset:16;

            uint64_t:29;

            bool is_watched:1;

            union {
                uint64_t read_section_id:14;
                uint64_t assign_location_id:14;
            } __attribute__((packed));

            /// Was this an assigned pointer (using `rcu_assign_pointer`) or a
            /// dereferenced pointer (using `rcu_dereference`).
            bool is_deref:1;            // high
        };

        uintptr_t as_uint;
        void *as_pointer;

    } __attribute__((packed));


    static_assert(sizeof(uint64_t) == sizeof(rcudbg_watched_address),
        "Invalid packing of `rcudbg_watched_address` structure.");


    /// Descriptor type for RCU debugging. Note: this object is never allocated!
    struct rcudbg_descriptor {
    private:
        inline rcudbg_descriptor(void) throw() { ASSERT(false); }

    public:

        /// Initialisation for an `rcu_dereference`d pointer.
        static bool allocate_and_init(
            rcudbg_descriptor *, // descriptor, unused
            uintptr_t &counter_index,
            uintptr_t, // inherited index, unused
            rcudbg_watched_address deref_address,
            const char *carat
        ) throw();


        /// Initialisation for an `rcu_assign_pointer`d pointer.
        static bool allocate_and_init(
            rcudbg_descriptor *, // descriptor, unused
            uintptr_t &counter_index,
            uintptr_t, // inherited index, unused
            rcudbg_watched_address assign_pointer,
            rcudbg_watched_address assigned_pointer,
            const char *carat
        ) throw();

        /// Unused by `rcudbg`.
        static void free(rcudbg_descriptor *, uintptr_t) throw() { }
        static void assign(rcudbg_descriptor *, uintptr_t) throw() { }
        static rcudbg_descriptor *access(uintptr_t index) throw() { return nullptr; }
    };


    /// Specify the descriptor type to the watchpoints instrumentation.
    namespace wp {

        /// Mark the `rcudbg` descriptors as using a combined allocation and
        /// initialisation mechanism.
        template <>
        struct allocate_and_init_descriptor<rcudbg_descriptor> {
            enum {
                VALUE = true
            };
        };

        template <typename>
        struct descriptor_type {
            typedef rcudbg_descriptor type;
        };
    }
}


#endif /* RCUDBG_DESCRIPTOR_H_ */
