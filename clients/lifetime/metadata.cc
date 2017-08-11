/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "clients/lifetime/metadata.h"

#include "granary/allocator.h"
#include "granary/hash_table.h"

using namespace granary;

namespace client {

// Statically allocated array of meta-data about objects. We store an object's
// base address
object_metadata DESCRIPTORS[wp::MAX_NUM_WATCHPOINTS];

// Boolean flag that tells us if we should even try to allocate descriptors for
// objects. We can run out of descriptors pretty quickly.
//
// TODO(akshay): Recycle descriptors by modifying `object_metadata::free`.
static std::atomic<bool> try_allocate(ATOMIC_VAR_INIT(true));

// Hash table mapping allocator return addresses to data structure descriptors.
// The idea here is that an (allocator, object size) pair generally represents
// a data structure.
//
// TODO(akshay): Future generalization, map a pair of a return address and
//               object size to the data structure metadata pointer.
//
// TODO(akshay): Future concurrency concern: guard this with a spin lock.
static static_data<hash_table<void *, data_structure_metadata *>> addr_to_ds;

// Because the hash table is wrapped up in a `static_data` template, we need
// to manually construct it *as if* it were just its plain old type.
//
// The `static_data` type serves as a dummy contained of proper size/alignment
// for some more complex data structure so that we can defer the data
// structure's initialization until long after some of the more "core" Granary
// components have been initialized. If we didn't have this deferral mechanism
// then we risk that the data structure is constructed too early, which leads
// to undefined behaviour, or it is never constructed (in the case of the
// kernel, which generally doesn't call static constructors when loading in
// modules!).
STATIC_INITIALISE_ID(lifetime_client, {
  addr_to_ds.construct();
})

// Return the index of some object_metadata in the DESCRIPTORS array.
inline static uintptr_t index_of(const object_metadata *desc) {
  return desc - &(DESCRIPTORS[0]);
}

// Initialize the `data_structure_metadata` structure.
//
// When we do something like:
//
//        void *x = malloc(99)
//    return_address:
//
// What will actually happen during instrumentation is:
//
//        void *x = wrapped_malloc(...)
//    cache_return_address:
//
// Where the `wrapped_malloc` is defined in `clients/lifetime/user_wrappers.h`,
// and does an `add_watchpoint` on the address that, when returned, will be
// assigned to `x`.
//
// When we add the watchpoint, we pass in the (1) address of the object being
// allocated, (2) the size of the object being allocated, and (3) the return
// address of the wrapper, which will point to the code following the call of
// `wrapped_malloc` in the code cache.
//
// Then we will arrange this data into the following structure:
//
//      `x` is a pointer, whose "watched" address is 0x000100000000abcd
//
//      The `0x0001` component of the address is a combination of the
//      "counter index" (0000, 15 bits) and a single, always-on `1` bit that
//      marks this address as being watched.
//
//      We take the counter index as an index into the `DESCRIPTORS` array.
//      This gets us object-specific meta-data about `x`. This looks like:
//
//            `x`        = 0x[0001]00000000abcd
//                              |
//                             \./
//  .- - - - - - -  DESCRIPTORS[0] - - - - - - -.
// /                                             |
// object_metadata{base_addr = 0x000100000000abcd,
//                 data_structure -.
//                                 |
//                         .-------'
//                         '->  data_structure_metadata{
//                                num_bytes = 99,
//                                num_reads -> [0, 0, ... 0],  // 99 counters
//                                num_writes -> [0, 0, ... 0],
//                                allocator_addres -> cache_return_address}
//
static data_structure_metadata *create_data_structure_meta(
    size_t size, void *allocator_address) {

  auto desc = allocate_memory<data_structure_metadata>();
  auto shadow = allocate_memory<unsigned>(size * 2);
  desc->num_bytes = size;
  desc->num_reads = shadow;
  desc->num_writes = &(shadow[size]);
  desc->allocator_address = allocator_address;
  return desc;
}

// Return some newly allocated object_metadata for an object.
static object_metadata *allocate(void *addr, size_t size,
                                 void *allocator_addr) {
  uintptr_t index = wp::next_counter_index(0);
  if(index < wp::MAX_NUM_WATCHPOINTS) {

    // Find the data-structure meta-data that we will associate with this
    // object. If we don't have any data structure meta-data associated with the
    // return address of our allocator then created the data-structure meta-data
    // on the spot.
    //
    // TODO(akshay): Synchronization.
    data_structure_metadata *ds_meta(nullptr);
    if (!addr_to_ds->load(allocator_addr, ds_meta)) {
      ds_meta = create_data_structure_meta(size, allocator_addr);
      addr_to_ds->store(allocator_addr, ds_meta);
    }

    // Find this object's meta-data based on the allocated "counter index".
    auto obj_meta = &(DESCRIPTORS[index]);

    // Initialize the `object_metadata` structure to point to its associated
    // data structure.
    obj_meta->base_addr = (uintptr_t) addr;
    obj_meta->data_structure = ds_meta;

    return obj_meta;

  } else {
    try_allocate.store(false, std::memory_order_release);
    return nullptr;
  }
}

// Allocate and initialize a watchpoint descriptor. For now we just allow a
// fixed number to be allocated.
bool object_metadata::allocate_and_init(
    object_metadata *&desc, uintptr_t &index, uintptr_t,
                                 void *addr_, size_t size_,
                                 void *allocator_addr_) {
  if (try_allocate.load(std::memory_order_acquire) &&
      (desc = allocate(addr_, size_, allocator_addr_))) {

    // Assign to the `index` out parameter. The value assigned here will taint
    // (i.e. replace) the 15 high-order bits of an address, and the 16th highest
    // order bit will be 0 (in user space) and 1 (in kernel space).
    index = index_of(desc);

    // Tell the watchpoints system that we were able to allocate a watchpoint
    // index for this address.
    return true;
  }
  return false;
}

// Free the descriptor.
void object_metadata::free(object_metadata *, uintptr_t) {
  // TODO(akshay): Implement me in the future.
}

// Get the descriptor of a watchpoint based on its index.
object_metadata *object_metadata::access(uintptr_t index) {
  return &(DESCRIPTORS[index]);
}

}  // namespace client
