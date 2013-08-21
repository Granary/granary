/*
 * descriptor.cc
 *
 *  Created on: 2013-08-05
 *      Author: akshayk
 */

#include "granary/types.h"
#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/clients/shadow_memory/descriptor.h"

using namespace granary;
using namespace granary::types;


namespace client { namespace wp {


    /// Set one or more of the state values for this object. To use this, do
    /// something like:
    ///
    ///     `state.set({{was_freed = true}});`
    ///
    /// To set the `was_freed` value.
    void shadow_policy_state::set_state(shadow_policy_state bits_to_set) throw() {
        uint8_t old_bits;
        uint8_t new_bits;
        do {
            old_bits = as_bits;
            new_bits = old_bits | bits_to_set.as_bits;
        } while(!__sync_bool_compare_and_swap(&as_bits, old_bits, new_bits));
    }


    /// Set one or more of the state values for this object. To use this, do
    /// something like:
    ///
    ///     `state.unset({{was_freed = true}});`
    ///
    /// To unset the `was_freed` value.
    void shadow_policy_state::unset_state(shadow_policy_state bits_to_unset) throw() {
        uint8_t old_bits;
        uint8_t new_bits;
        do {
            old_bits = as_bits;
            new_bits = old_bits & (~bits_to_unset.as_bits);
        } while(!__sync_bool_compare_and_swap(&as_bits, old_bits, new_bits));
    }


    /// Configuration for shadow policy descriptors.
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


    /// Allocator for the shadow policy descriptors.
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
    shadow_policy_descriptor *DESCRIPTORS[
        client::wp::MAX_NUM_WATCHPOINTS
    ] = {nullptr};


    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool shadow_policy_descriptor::allocate(
        shadow_policy_descriptor *&desc,
        uintptr_t &counter_index,
        const uintptr_t
    ) throw() {
        counter_index = 0;
        desc = nullptr;


        // Try to pull a descriptor off of a CPU-private free list.
        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        shadow_policy_descriptor *&free_list(state->free_list);
        desc = free_list;
        if(desc) {
            free_list = desc->next_free;
        }
        IF_KERNEL( granary_store_flags(flags); )

        counter_index = 0;

        // We got a descriptor from the free list.
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

            desc = DESCRIPTOR_ALLOCATOR->allocate<shadow_policy_descriptor>();
        }

        ASSERT(counter_index <= client::wp::MAX_COUNTER_INDEX);

        memset(desc, 0, sizeof *desc);

        desc->state.is_active = true;

        return true;
    }

    inline unsigned long normalize_addr(unsigned long u)
    {
        return (signed long)(u << 16) >> 16;
    }


    /// Initialise a watchpoint descriptor.
    void shadow_policy_descriptor::init(
        shadow_policy_descriptor *desc,
        void *base_address,
        size_t size
    ) throw() {
        unsigned long shadow;
        desc->base_address = unwatched_address(unsafe_cast<uintptr_t>(base_address));

        desc->size = size;
        shadow = unsafe_cast<unsigned long>(types::__kmalloc(size/4, 0x20));
        memset(unsafe_cast<void*>(shadow), 0x0, size/4);

        desc->read_shadow = unsafe_cast<app_pc>(shadow);

        desc->write_shadow = unsafe_cast<app_pc>(shadow+size/8);
    }


    /// Notify the shadow policy that the descriptor can be assigned to
    /// the index.
    void shadow_policy_descriptor::assign(
        shadow_policy_descriptor *desc,
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
    shadow_policy_descriptor *shadow_policy_descriptor::access(
        uintptr_t index
    ) throw() {
        ASSERT(index < client::wp::MAX_NUM_WATCHPOINTS);
        return DESCRIPTORS[index];
    }


    /// Free a watchpoint descriptor by adding it to a CPU-private free list.
    void shadow_policy_descriptor::free(
        shadow_policy_descriptor *desc,
        uintptr_t index
    ) throw() {
        shadow_policy_state desc_state;

        if(!is_valid_address(desc)) {
            return;
        }

        // Zero out the associated memory. Only do this when the descriptor is
        // freed because that's the point where we know that the memory
        // shouldn't be re-used.
        const uintptr_t base_address_(desc->base_address);
        memset(
            reinterpret_cast<void *>(
                base_address_ | shadow_policy_descriptor::BASE_ADDRESS_MASK),
            0,
            desc->size);

        // Reset the descriptor.
        memset(desc, 0, sizeof *desc);
        desc->index = index;

        desc_state.was_freed = true;
        desc_state.is_active = false;

        desc->state.set_state(desc_state);

        // Add the descriptor to the free list.
        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        shadow_policy_descriptor *&free_list(state->free_list);
        desc->next_free = free_list;
        free_list = desc;
        IF_KERNEL( granary_store_flags(flags); )
    }

}}

