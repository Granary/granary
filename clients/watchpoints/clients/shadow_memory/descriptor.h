/*
 * descriptor.h
 *
 *  Created on: 2013-08-05
 *      Author: akshayk
 */

#ifndef _SHADOW_POLICY_DESCRIPTORS_H_
#define _SHADOW_POLICY_DESCRIPTORS_H_

#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/clients/shadow_memory/kernel/linux/kernel_type_id.h"

#define NUM_SUB_STRUCTURES 4


namespace client { namespace wp {


    struct sub_struct_shadow_policy {
        uint16_t type_id;
        uint16_t byte_offset;
    } __attribute__((packed));

    /// State for a tracked object.
    union shadow_policy_state {
        struct {
            /// True if the watched object was accessed in the last epoch
            bool accessed_in_last_epoch;

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
            struct {
                // selective shadow memory for read operation
                granary::app_pc read_shadow;

                // shadow memory for the write operation
                granary::app_pc write_shadow;
            }__attribute__((packed));


            sub_struct_shadow_policy sub_structures[4];
            union {
                struct {

                    /// State of the object.
                    shadow_policy_state state;

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
                    uint16_t type;

                    /// object's base address.
                    uintptr_t base_address;
                };

                struct {
                    /// The combined index of this descriptor.
                    uint64_t index;

                    /// Pointer to the next free descriptor.
                    shadow_policy_descriptor *next_free;
                };
            };

        }__attribute__((packed));

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


        template <typename T>
        void update_type(T *ptr_) throw() {
            const uintptr_t ptr(reinterpret_cast<uintptr_t>(unwatched_address(ptr_)));
            const uint16_t begin(ptr - base_address);
            const uint16_t end(begin + size);

            type = type_id<struct inode>::VALUE;

            for(unsigned i(0); i < NUM_SUB_STRUCTURES; ++i) {
                sub_struct_shadow_policy &sub(sub_structures[i]);

                if(!sub.type_id) {
                    if(i) {
                        // Does the previous entry contain us? If so then ignore, it can be
                        // added in post-processing.
                        const sub_struct_shadow_policy &prev_sub(sub_structures[i - 1]);
                        if(end <= (prev_sub.byte_offset + TYPE_SIZES[prev_sub.type_id])) {
                            break;
                        }
                    }

                    goto overwrite;
                }

                // Already there; stop.
                if(type_id<T>::VALUE == sub.type_id && begin == sub.byte_offset) {
                    break;
                }

                // Containment: type T fully contains what's already there, overwrite it.
                if(sub.byte_offset >= begin
                && end >= (sub.byte_offset + TYPE_SIZES[sub.type_id])) {
                    goto overwrite;
                }

                continue;

            overwrite:
                sub.type_id = type_id<T>::VALUE;
                //sub.byte_offset = offset;
                break;
            }
        }


    } __attribute__((packed));

    /// Specify the descriptor type to the generic watchpoint framework.
    template <typename>
    struct descriptor_type {
        typedef shadow_policy_descriptor type;
        enum {
            ALLOC_AND_INIT = false,
            REINIT_WATCHED_POINTERS = false
        };
    };

    uint16_t get_inode_type_id(void){
        return type_id<struct inode>::VALUE;
    }
}}


#endif /* DESCRIPTOR_H_ */
