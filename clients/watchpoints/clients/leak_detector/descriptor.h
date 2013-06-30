/*
 * descriptors.h
 *
 *  Created on: 2013-06-23
 *      Author: akshayk, pgoodman
 */

#ifndef _LEAK_POLICY_DESCRIPTORS_H_
#define _LEAK_POLICY_DESCRIPTORS_H_

#include "clients/watchpoints/instrument.h"

namespace client { namespace wp {

    /// State for a tracked object.
    union leak_object_state {
        struct {

            /// Do we know the type of this object?
            bool type_is_known:1;

            /// Has app or host code freed this object? If so, then who freed
            /// this object?
            bool was_freed:1;
            bool was_freed_by_app:1;

            /// In which direction have we observed this object crossing the
            /// app/host boundary.
            bool crossed_host_boundary:1;
            bool crossed_app_boundary:1;

            /// Was this object allocated by app (instrumented) code? If so then
            /// this descriptor is likely in our descriptor table and associated
            /// with a watchpoint, as opposed to being "dangling".
            bool was_allocated_by_app:1;

            bool is_active:1;

        } __attribute__((packed));


        /// So that we can modify these states atomically.
        uint8_t as_bits;


        /// Set one or more of the state values for this object. To use this, do
        /// something like:
        ///
        ///     `state.set({{was_freed = true}});`
        ///
        /// To set the `was_freed` value.
        void set_state(leak_object_state bits_to_set) throw();


        /// Set one or more of the state values for this object. To use this, do
        /// something like:
        ///
        ///     `state.unset({{was_freed = true}});`
        ///
        /// To unset the `was_freed` value.
        void unset_state(leak_object_state bits_to_unset) throw();

    } __attribute__((packed));


    /// Specifies the leak_detector for the watched object.
    struct leak_detector_descriptor {

        enum : uint64_t {
            BASE_ADDRESS_MASK = (~0ULL << 48)
        };

        union {
            struct {

                /// True iff the watched object was accessed in the last
                /// epoch.
                bool accessed_in_last_epoch;

                /// Number of epochs since this object was last accessed.
                uint8_t num_epochs_since_last_access;

                /// Size in bytes of the allocated object. The limit address of
                /// the object of `base_address + size`.
                uint16_t size;

                /// Unique ID for the kernel type of the object. If we don't
                /// know the kernel type (assuming there is one) for this object
                /// then `type_id` will be `UNKNOWN_TYPE_ID`.
                ///
                /// If the type is known then we can use the type's size info
                /// in conjunction with the allocation size to find arrays of
                /// objects.
                uint16_t type_id;

                /// State of the object.
                leak_object_state state;

                /// Low 48 bits of this object's base address.
                uintptr_t base_address:48;

            } __attribute__((packed));


            struct {

                /// Pointer to the next free descriptor.
                leak_detector_descriptor *next_free;

                /// The combined index of this descriptor.
                uintptr_t index;
            };
        };

        /// Allocate a watchpoint descriptor.
        static bool allocate(
            leak_detector_descriptor *&,
            uintptr_t &,
            const uintptr_t
        ) throw();


        /// Free a watchpoint descriptor.
        static void free(leak_detector_descriptor *, uintptr_t) throw();


        /// Initialise a watchpoint descriptor.
        static void init(
            leak_detector_descriptor *,
            void *base_address,
            size_t size
        ) throw();


        /// Notify the leak_detectors policy that the descriptor can be assigned
        /// to the index.
        static void assign(
            leak_detector_descriptor *desc,
            uintptr_t index
        ) throw();


        /// Get the assigned descriptor for a given index.
        static leak_detector_descriptor *access(uintptr_t index) throw();

    } __attribute__((packed));


    /// Specify the descriptor type to the generic watchpoint framework.
    template <typename>
    struct descriptor_type {
        typedef leak_detector_descriptor type;
    };

}}

#endif /* _LEAK_POLICY_DESCRIPTORS_H_ */
