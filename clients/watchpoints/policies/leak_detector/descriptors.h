/*
 * descriptors.h
 *
 *  Created on: 2013-06-23
 *      Author: akshayk
 */

#ifndef _LEAK_POLICY_DESCRIPTORS_H_
#define _LEAK_POLICY_DESCRIPTORS_H_

#include "clients/watchpoints/instrument.h"

namespace client { namespace wp {

    ///Specifies the leak_detector for the watched object.
    struct leak_detector_descriptor {

        enum : uint64_t {
            FREE_LIST_END = ~static_cast<uint64_t>(0ULL)
        };

        union {
            struct {
                /// Most objects won't be more than 16 pages big, so an
                /// m16&16 parameter suffices (as opposed to m32&32).
                uint32_t lower_bound;
                uint32_t upper_bound;
            };

            /// Descriptor index of the next-freed object.
            uint64_t next_free_index;
        };

        ///State of watchpoint for leak detector
        uint32_t state;

        /// Descriptor index of this descriptor within the descriptor table.
        uint32_t my_index;

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

        /// Notify the leak_detectors policy that the descriptor can be assigned to
        /// the index.
        static void assign(leak_detector_descriptor *desc, uintptr_t index) throw();


        /// Get the assigned descriptor for a given index.
        static leak_detector_descriptor *access(uintptr_t index) throw();

    } __attribute__((packed));


    /// Specify the descriptor type to the generic watchpoint framework.
    template <typename>
    struct descriptor_type {
        typedef leak_detector_descriptor type;
    };

}
}

#endif /* DESCRIPTORS_H_ */
