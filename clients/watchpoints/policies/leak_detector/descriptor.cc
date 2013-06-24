/*
 * descriptors.cc
 *
 *  Created on: 2013-06-23
 *      Author: akshayk, pgoodman
 */


#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/policies/leak_detector/descriptor.h"

using namespace granary;


namespace client { namespace wp {


    /// Set one or more of the state values for this object. To use this, do
    /// something like:
    ///
    ///     `state.set({{was_freed = true}});`
    ///
    /// To set the `was_freed` value.
    void leak_object_state::set(leak_object_state bits_to_set) throw() {
        uint8_t old_bits;
        uint8_t new_bits;
        do {
            old_bits = as_bits;
            new_bits = old_bits | bits_to_set.as_bits;
        } while(__sync_bool_compare_and_swap(&as_bits, old_bits, new_bits));
    }


    /// Set one or more of the state values for this object. To use this, do
    /// something like:
    ///
    ///     `state.unset({{was_freed = true}});`
    ///
    /// To unset the `was_freed` value.
    void leak_object_state::unset(leak_object_state bits_to_unset) throw() {
        uint8_t old_bits;
        uint8_t new_bits;
        do {
            old_bits = as_bits;
            new_bits = old_bits & (~bits_to_unset.as_bits);
        } while(__sync_bool_compare_and_swap(&as_bits, old_bits, new_bits));
    }


    /// Configuration for leak detector descriptors.
    struct descriptor_allocator_config {
        enum {
            SLAB_SIZE = 2 * granary::PAGE_SIZE,
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
    leak_detector_descriptor *DESCRIPTORS[
        client::wp::MAX_NUM_WATCHPOINTS
    ] = {nullptr};


    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool leak_detector_descriptor::allocate(
        leak_detector_descriptor *&desc,
        uintptr_t &counter_index,
        const uintptr_t
    ) throw() {
        counter_index = 0;
        desc = nullptr;

        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        leak_detector_descriptor *&free_list(state->free_list);
        if(free_list) {
            desc = free_list;
            free_list = desc->next_free;
        }

        IF_KERNEL( granary_store_flags(flags); )

        counter_index = 0;
        if(desc) {
            uintptr_t inherited_index_;

            client::wp::destructure_combined_index(
                desc->index, counter_index, inherited_index_);

        // Try to allocate one.
        } else {
            counter_index = client::wp::next_counter_index();
            if(counter_index > client::wp::MAX_COUNTER_INDEX) {
                return false;
            }

            desc = DESCRIPTOR_ALLOCATOR->allocate<leak_detector_descriptor>();
        }

        ASSERT(counter_index <= client::wp::MAX_COUNTER_INDEX);

        memset(desc, 0, sizeof *desc);

        return true;
    }


    /// Initialise a watchpoint descriptor.
    void leak_detector_descriptor::init(
        leak_detector_descriptor *desc,
        void *base_address,
        size_t size
    ) throw() {
        desc->base_address = reinterpret_cast<uintptr_t>(base_address);
        desc->size = size;
    }


    /// Notify the leak_detectors policy that the descriptor can be assigned to
    /// the index.
    void leak_detector_descriptor::assign(
        leak_detector_descriptor *desc,
        uintptr_t index
    ) throw() {
        ASSERT(index < MAX_NUM_WATCHPOINTS);

        // Here is where we really know that this descriptor is for an app-
        // allocated object (watched) as opposed to a tracked host-allocated
        // object (unwatched).
        desc->state.was_allocated_by_app = true;

        DESCRIPTORS[index] = desc;
    }


    /// Get the descriptor of a watchpoint based on its index.
    leak_detector_descriptor *leak_detector_descriptor::access(
        uintptr_t index
    ) throw() {
        ASSERT(index < client::wp::MAX_NUM_WATCHPOINTS);
        return DESCRIPTORS[index];
    }


    /// Free a watchpoint descriptor by adding it to a CPU-private free list.
    void leak_detector_descriptor::free(
        leak_detector_descriptor *desc,
        uintptr_t index
    ) throw() {
        if(!is_valid_address(desc)) {
            return;
        }

        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        leak_detector_descriptor *&free_list(state->free_list);

        desc->index = index;
        desc->next_free = free_list;
        free_list = desc;

        IF_KERNEL( granary_store_flags(flags); )
    }
}}
