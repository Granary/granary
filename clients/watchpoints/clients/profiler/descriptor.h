/*
 * descriptors.h
 *
 *  Created on: 2013-06-23
 *      Author: akshayk, pgoodman
 */

#ifndef _WATCHPOINT_PROFILER_DESCRIPTORS_H_
#define _WATCHPOINT_PROFILER_DESCRIPTORS_H_

#include "clients/watchpoints/instrument.h"

enum object_source {
    SOURCE_KMALLOC=1,
    SOURCE_KMALLOC_TRACK_CALLER,
    SOURCE_KMALLOC_NODE,
    SOURCE_KMALLOC_NODE_TRACK_CALLER,
    SOURCE_KREALLOC,
    SOURCE_KMEM_CACHE_ALLOC,
    SOURCE_KMEM_CACHE_ALLOC_NODE,
    SOURCE__GET_FREE_PAGES,
    SOURCE__GET_ZEROED_PAGES,
    SOURCE_ALLOC_PAGES_EXACT,
    SOURCE_ALLOC_PAGES_EXACT_NID
};

namespace client { namespace wp {

    struct profiler_descriptor {

        enum : uint64_t {
            BASE_ADDRESS_MASK = (~0ULL << 48)
        };

        struct {

            uintptr_t module_access_counter;

            uintptr_t kernel_access_counter;

            uintptr_t object_source;

            union {
                struct {

                    /// Size in bytes of the allocated object. The limit address of
                     /// the object of `base_address + size`.
                    uint16_t size;

                    uint16_t type_id;

                    uintptr_t base_address;
                };

                struct {
                    /// The combined index of this descriptor.
                    uint32_t index;

                    /// Pointer to the next free descriptor.
                    profiler_descriptor *next_free;
                };
            };

        };

        /// Allocate a watchpoint descriptor.
        static bool allocate(
            profiler_descriptor *&,
            uintptr_t &,
            const uintptr_t
        ) throw();


        /// Free a watchpoint descriptor.
        static void free(profiler_descriptor *, uintptr_t) throw();


        /// Initialise a watchpoint descriptor.
        static void init(
            profiler_descriptor *,
            void *base_address,
            size_t size,
            uintptr_t source
        ) throw();


        static void assign(
            profiler_descriptor *desc,
            uintptr_t index
        ) throw();


        /// Get the assigned descriptor for a given index.
        static profiler_descriptor *access(uintptr_t index) throw();

    } __attribute__((packed));


    /// Specify the descriptor type to the generic watchpoint framework.
    template <typename>
    struct descriptor_type {
        typedef profiler_descriptor type;
    };

}}

#endif /* _LEAK_POLICY_DESCRIPTORS_H_ */
