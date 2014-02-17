/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "clients/lifetime/metadata.h"

using namespace granary;

namespace client {

metadata DESCRIPTORS[wp::MAX_NUM_WATCHPOINTS];

static std::atomic<bool> try_allocate(ATOMIC_VAR_INIT(true));

// Return the index of some metadata in the DESCRIPTORS array.
inline static uintptr_t index_of(const metadata *desc) throw() {
  return desc - &(DESCRIPTORS[0]);
}

// Return some newly allocated metadata for an object.
static metadata *allocate(void) throw() {
  auto index = wp::next_counter_index(0);
  if(index < wp::MAX_NUM_WATCHPOINTS) {
    return &(DESCRIPTORS[index]);
  } else {
    try_allocate.store(false, std::memory_order_release);
    return nullptr;
  }
}

// Allocate and initialize a watchpoint descriptor. For now we just allow a
// fixed number to be allocated.
bool metadata::allocate_and_init(metadata *&desc, uintptr_t &index, uintptr_t,
                                 void *addr_, size_t size_,
                                 void *allocator_addr_) throw() {
  if (try_allocate.load(std::memory_order_acquire) && (desc = allocate())) {
    index = index_of(desc);
    desc->addr = addr_;
    desc->size = size_;
    desc->allocator_addr = allocator_addr_;
    return true;
  }
  return false;
}

// Get the descriptor of a watchpoint based on its index.
metadata *metadata::access(uintptr_t index) throw() {
  return &(DESCRIPTORS[index]);
}

}  // namespace client
