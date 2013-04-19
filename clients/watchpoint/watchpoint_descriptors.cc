/*
 * watchpoint_descriptors.cc
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#include <new>
#include "granary/client.h"
#include "clients/watchpoint/descriptors/descriptor_table.h"
#include "clients/watchpoint/descriptors/wrapper_interface.h"

namespace client {

    descriptor *descriptor_table::table_entries[descriptor_table::NUM_WATCHPOINT] = {nullptr};

    unsigned long descriptor_table::table_index = 0UL;

    granary::bump_pointer_allocator<descriptors_config>
    descriptor_table::allocator;

#if GRANARY_IN_KERNEL
    STATIC_INITIALISE_ID(watchpoint_descriptor_table, {
        new (&(descriptor_table::allocator)) decltype(descriptor_table::allocator);
    })
#endif
}
