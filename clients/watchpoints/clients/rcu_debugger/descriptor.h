/*
 * descriptor.h
 *
 *  Created on: 2013-08-09
 *      Author: akshayk
 */

#ifndef _RCU_DESCRIPTOR_H_
#define _RCU_DESCRIPTOR_H_

#include "clients/watchpoints/instrument.h"

namespace client { namespace wp {

    /// State for a rcu allocated object.
    union rcu_policy_state {
        struct {
            /// True if the watched object was accessed in the last epoch
            bool accessed_in_last_epoch;

            /// Do we know the type of this object?
            bool type_is_known:1;

            /// Has app or host code freed this object? If so, then who freed
            /// this object?
            bool was_freed:1;

            /// Was this object allocated by app (instrumented) code? If so then
            /// this descriptor is likely in our descriptor table and associated
            /// with a watchpoint, as opposed to being "dangling".
            bool was_allocated_by_app:1;

            /// If the object currently is active;
            bool is_active:1;

            /// true if the object is rcu protected object
            /// decided at rcu_watch_assign_pointers;
            /// rcu policy applies to only rcu protected object;
            bool is_rcu_object:1;

            //gets set inside rcu dereference
            bool is_rcu_dereference:1;

        } __attribute__((packed));


        /// So that we can modify these states atomically.
        uint8_t as_bits;


        /// Set one or more of the state values for this object. To use this, do
        /// something like:
        ///
        ///     `state.set({{was_freed = true}});`
        ///
        /// To set the `was_freed` value.
        void set_state(rcu_policy_state bits_to_set) throw();


        /// Set one or more of the state values for this object. To use this, do
        /// something like:
        ///
        ///     `state.unset({{was_freed = true}});`
        ///
        /// To unset the `was_freed` value.
        void unset_state(rcu_policy_state bits_to_unset) throw();

    } __attribute__((packed));


    /// Specifies the rcu policy descriptor for the watched object.
    struct rcu_policy_descriptor {

        enum : uint64_t {
            BASE_ADDRESS_MASK = (~0ULL << 48)
        };

        struct {

            union {
                struct {

                    /// State of the object.
                    rcu_policy_state state;

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

                    /// object's base address.
                    uintptr_t base_address;
                };

                struct {
                    /// The combined index of this descriptor.
                    uint64_t index;

                    /// Pointer to the next free descriptor.
                    rcu_policy_descriptor *next_free;
                };

                rcu_policy_descriptor *list_next;

            };

        }__attribute__((packed));

        /// Allocate a watchpoint descriptor.
        static bool allocate(
                rcu_policy_descriptor *&,
                uintptr_t &,
                const uintptr_t
        ) throw();


        /// Free a watchpoint descriptor.
        static void free(rcu_policy_descriptor *, uintptr_t) throw();


        /// Initialise a watchpoint descriptor.
        static void init(
                rcu_policy_descriptor *,
                void *base_address,
                size_t size
        ) throw();


        /// Notify the rcu policy that the descriptor can be assigned
        /// to the index.
        static void assign(
                rcu_policy_descriptor *desc,
                uintptr_t index
        ) throw();


        /// Get the assigned descriptor for a given index.
        static rcu_policy_descriptor *access(uintptr_t index) throw();

    } __attribute__((packed));

    /// Specify the descriptor type to the generic watchpoint framework.
    template <typename>
    struct descriptor_type {
        typedef rcu_policy_descriptor type;
    };

}}




#endif /* DESCRIPTOR_H_ */
