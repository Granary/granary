/*
 * watchpoint_meta.h
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#ifndef _DESCRIPTOR_TABLE_H_
#define _DESCRIPTOR_TABLE_H_

#include "granary/globals.h"

#include "clients/watchpoint/descriptors/descriptor.h"

namespace client {


    struct descriptors_config {
        enum {
            SLAB_SIZE = 4 * granary::PAGE_SIZE,
            EXECUTABLE = false,
            TRANSIENT = false,
            SHARED = true,
            EXEC_WHERE = granary::EXEC_NONE,
            MIN_ALIGN = 16
        };
    };


    struct descriptor_table {
    public:
        enum {
            NUM_WATCHPOINT = 4096
        };

        static descriptor *table_entries[NUM_WATCHPOINT];

        static granary::bump_pointer_allocator<descriptors_config> allocator;

        static unsigned long table_index;

        template <typename T>
        static descriptor *allocate(T *address, unsigned long size){
            descriptor *entry(allocator.allocate<descriptor>());
            entry->base_address = (uint64_t)address;
            entry->limit = (uint64_t)address + size;
            return entry;
        }

        template <typename T>
        static void release(T *address){
            (void)release;
        }

        template <typename T>
        static uint64_t get_index(){
            return descriptor_table::table_index;
        }

        template<typename T>
        static uint64_t allocate_descriptor(T* address, uint64_t size){
            descriptor *table_entry = allocate<T>(address, size);
            descriptor_table::table_entries[table_index++] = table_entry;
            return descriptor_table::table_index;
        }

    };

}


#endif /* _DESCRIPTOR_TABLE_H_ */
