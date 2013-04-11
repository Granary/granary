/*
 * watchpoint_meta.h
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#ifndef _DESCRIPTOR_TABLE_H_
#define _DESCRIPTOR_TABLE_H_

#include "clients/watchpoint/descriptors/descriptor.h"

namespace client {

struct descriptor_table {
public:
	enum {
		NUM_WATCHPOINT = 4096,
	};

	static struct descriptors* table_entries[NUM_WATCHPOINT];

	typedef bump_pointer_allocator<descriptors_config> region_allocator_type;

	region_allocator_type allocator;
	unsigned long table_index;

	template <typename T>
	descriptors *allocate(T *address, unsigned long size){
		descriptors *entry(allocator.allocate<T>());
		entry->base_address = (uint64_t)address;
		entry->limit = (uint64_t)address + size;
		return entry;
	}

	template <typename T>
	void release(T *address){
		(void)release;
	}

	template <typename T>
	uint64_t get_index(){
		return table_index;
	}

	template<typename T>
	uint64_t allocate_descriptor(T* address, uint64_t size){
		descriptors *table_entry = allocate<T>(address, size);
		table_entries[table_index++] = table_entry;
		return table_index;
	}

};

}


#endif /* _DESCRIPTOR_TABLE_H_ */
