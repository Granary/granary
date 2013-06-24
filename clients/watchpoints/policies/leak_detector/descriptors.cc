/*
 * descriptors.cc
 *
 *  Created on: 2013-06-23
 *      Author: akshayk
 */


#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/policies/leak_detector/descriptors.h"

using namespace granary;

#define ENABLE_DESCRIPTORS 1

namespace client { namespace wp {

    /// Configuration for bound descriptors.
    struct descriptor_allocator_config {
        enum {
            SLAB_SIZE = granary::PAGE_SIZE,
            EXECUTABLE = false,
            TRANSIENT = false,
            SHARED = true,
            EXEC_WHERE = granary::EXEC_NONE,
            MIN_ALIGN = 4
        };
    };


    /// Allocator for the leak_detector descriptors.
    static granary::static_data<
        granary::bump_pointer_allocator<descriptor_allocator_config>
    > DESCRIPTOR_ALLOCATOR;


    /// Initialise the descriptor allocator.
    STATIC_INITIALISE({
        DESCRIPTOR_ALLOCATOR.construct();
    })

    /// Pointers to the descriptors.
    ///
    /// Note: Note `static` so that we can access by the mangled name in
    ///       x86/bound_policy.asm.
    leak_detector_descriptor *DESCRIPTORS[client::wp::MAX_NUM_WATCHPOINTS] = {nullptr};

    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool leak_detector_descriptor::allocate(
        leak_detector_descriptor *&desc,
        uintptr_t &counter_index,
        const uintptr_t
    ) throw() {
        counter_index = 0;
        desc = nullptr;

        counter_index = 0;
        if(desc) {
            uintptr_t inherited_index_;
            client::wp::destructure_combined_index(
                desc->my_index, counter_index, inherited_index_);

        // Try to allocate one.
        } else {
            counter_index = client::wp::next_counter_index();
            if(counter_index > client::wp::MAX_COUNTER_INDEX) {
                return false;
            }

            desc = DESCRIPTOR_ALLOCATOR->allocate<leak_detector_descriptor>();
        }

        ASSERT(counter_index <= client::wp::MAX_COUNTER_INDEX);

        return true;
    }


    /// Initialise a watchpoint descriptor.
    void leak_detector_descriptor::init(
        leak_detector_descriptor *desc,
        void *base_address,
        size_t size
    ) throw() {
        const uintptr_t base(reinterpret_cast<uintptr_t>(base_address));
        desc->lower_bound = static_cast<uint32_t>(base);
        desc->upper_bound = static_cast<uint32_t>(base + size);
    }


    /// Notify the leak_detectors policy that the descriptor can be assigned to
    /// the index.
    void leak_detector_descriptor::assign(
        leak_detector_descriptor *desc,
        uintptr_t index
    ) throw() {
        ASSERT(index < MAX_NUM_WATCHPOINTS);
        desc->my_index = index;
        DESCRIPTORS[index] = desc;
    }


    /// Get the descriptor of a watchpoint based on its index.
    leak_detector_descriptor *leak_detector_descriptor::access(
        uintptr_t index
    ) throw() {
        ASSERT(index < client::wp::MAX_NUM_WATCHPOINTS);
        return DESCRIPTORS[index];
    }


    /// Free a watchpoint descriptor by adding it to a free list.
    void leak_detector_descriptor::free(
        leak_detector_descriptor *desc,
        uintptr_t index
    ) throw() {
        if(!is_valid_address(desc)) {
            return;
        }
        ASSERT(index == desc->my_index);
    }


}
}
