/*
 * descriptors.cc
 *
 *  Created on: 2013-06-23
 *      Author: akshayk, pgoodman
 */


#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/clients/profiler/descriptor.h"

using namespace granary;

unsigned long app_access_counter[120000] = {0ULL,};
unsigned long host_access_counter[120000] = {0ULL,};


namespace client { namespace wp {


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
    profiler_descriptor *DESCRIPTORS[
        client::wp::MAX_NUM_WATCHPOINTS
    ] = {nullptr};



    /// Allocate a watchpoint descriptor and assign `desc` and `index`
    /// appropriately.
    bool profiler_descriptor::allocate(
        profiler_descriptor *&desc,
        uintptr_t &counter_index,
        const uintptr_t
    ) throw() {

        counter_index = 0;
        desc = nullptr;

        // Try to pull a descriptor off of a CPU-private free list.
        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        profiler_descriptor *&free_list(state->free_list);
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

            desc = DESCRIPTOR_ALLOCATOR->allocate<profiler_descriptor>();
        }

        ASSERT(counter_index <= client::wp::MAX_COUNTER_INDEX);

        memset(desc, 0, sizeof *desc);

        return true;
    }


    /// Initialise a watchpoint descriptor.
    void profiler_descriptor::init(
        profiler_descriptor *desc,
        void *base_address,
        size_t size,
        uintptr_t source
    ) throw() {
        desc->base_address = reinterpret_cast<uintptr_t>(base_address);
        desc->size = size;
        desc->object_source = source;
    }


    void profiler_descriptor::assign(
        profiler_descriptor *desc,
        uintptr_t index
    ) throw() {
        ASSERT(index < MAX_NUM_WATCHPOINTS);

        DESCRIPTORS[index] = desc;
    }


    /// Get the descriptor of a watchpoint based on its index.
    profiler_descriptor *profiler_descriptor::access(
        uintptr_t index
    ) throw() {
        ASSERT(index < client::wp::MAX_NUM_WATCHPOINTS);
        return DESCRIPTORS[index];
    }


    /// Free a watchpoint descriptor by adding it to a CPU-private free list.
    void profiler_descriptor::free(
        profiler_descriptor *desc,
        uintptr_t index
    ) throw() {

        if(!is_valid_address(desc)) {
            return;
        }

        app_access_counter[desc->object_source*10000+desc->size] += desc->module_access_counter;
        host_access_counter[desc->object_source*10000+desc->size] += desc->kernel_access_counter;

        //granary::printf("freed: source (%llx) app_count(%llx), host_count(%llx), size(%llx)\n",
          //                  desc->object_source, desc->module_access_counter, desc->kernel_access_counter, desc->size);


        // Zero out the associated memory. Only do this when the descriptor is
        // freed because that's the point where we know that the memory
        // shouldn't be re-used.
        const uintptr_t base_address_(desc->base_address);
        memset(
            reinterpret_cast<void *>(
                base_address_ | profiler_descriptor::BASE_ADDRESS_MASK),
            0,
            desc->size);

        // Reset the descriptor.
        memset(desc, 0, sizeof *desc);
        desc->index = index;

        // Add the descriptor to the free list.
        IF_KERNEL( eflags flags(granary_disable_interrupts()); )
        cpu_state_handle state;
        profiler_descriptor *&free_list(state->free_list);
        desc->next_free = free_list;
        free_list = desc;
        IF_KERNEL( granary_store_flags(flags); )
    }
}}
