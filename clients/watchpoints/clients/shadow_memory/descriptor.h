/*
 * descriptor.h
 *
 *  Created on: 2013-08-05
 *      Author: akshayk
 */

#ifndef _SHADOW_POLICY_DESCRIPTORS_H_
#define _SHADOW_POLICY_DESCRIPTORS_H_

#include "clients/watchpoints/instrument.h"

namespace client { namespace wp {

    /// State for a tracked object.
    union shadow_policy_state {
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
        void set_state(shadow_policy_state bits_to_set) throw();


        /// Set one or more of the state values for this object. To use this, do
        /// something like:
        ///
        ///     `state.unset({{was_freed = true}});`
        ///
        /// To unset the `was_freed` value.
        void unset_state(shadow_policy_state bits_to_unset) throw();

    } __attribute__((packed));


    /// Specifies the shadow policy descriptor for the watched object.
    struct shadow_policy_descriptor {

        enum : uint64_t {
            BASE_ADDRESS_MASK = (~0ULL << 48)
        };

        struct {

            /// State of the object.
            shadow_policy_state state;

            union {
                struct {

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

                    /// Low 48 bits of this object's base address.
                    uintptr_t base_address:48;
                };

                struct {
                    /// The combined index of this descriptor.
                    uint32_t index;

                    /// Pointer to the next free descriptor.
                    shadow_policy_descriptor *next_free;
                };
            };

            struct {

                // selective shadow memory for read operation
                granary::app_pc read_shadow;

                // shadow memory for the write operation
                granary::app_pc write_shadow;
            }__attribute__((packed));

        };

        /// Allocate a watchpoint descriptor.
        static bool allocate(
                shadow_policy_descriptor *&,
                uintptr_t &,
                const uintptr_t
        ) throw();


        /// Free a watchpoint descriptor.
        static void free(shadow_policy_descriptor *, uintptr_t) throw();


        /// Initialise a watchpoint descriptor.
        static void init(
                shadow_policy_descriptor *,
                void *base_address,
                size_t size
        ) throw();


        /// Notify the shadow policy that the descriptor can be assigned
        /// to the index.
        static void assign(
                shadow_policy_descriptor *desc,
                uintptr_t index
        ) throw();


        /// Get the assigned descriptor for a given index.
        static shadow_policy_descriptor *access(uintptr_t index) throw();

    } __attribute__((packed));

    /// Specify the descriptor type to the generic watchpoint framework.
    template <typename>
    struct descriptor_type {
        typedef shadow_policy_descriptor type;
    };
}}


#endif /* DESCRIPTOR_H_ */
