/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef CLIENTS_LIFETIME_METADATA_H_
#define CLIENTS_LIFETIME_METADATA_H_

#include "clients/watchpoints/instrument.h"

namespace client {

// Meta-data about data structures themselves.
struct data_structure_metadata {
  // Size of the data-structure in bytes.
  unsigned num_bytes;

  // Number of reads/writes to the various bytes of the data-structures.
  unsigned *num_reads;
  unsigned *num_writes;

  void *allocator_address;
};

// Meta-data about allocated objects.
struct object_metadata {
  uintptr_t base_addr;
  data_structure_metadata *data_structure;

  // IGNORE STUFF BELOW HERE: it's just watchpoints-specific stuff.

  static bool allocate_and_init(
      // Fixed arguments required by the watchpoints system.
      object_metadata *&desc,
      uintptr_t &counter_index,
      const uintptr_t inherited_index,

      // Arguments, you can add more, but make sure they match invocations
      // to `add_watchpoint`.
      void *addr_,
      size_t size_,
      void *allocator_addr_) throw();

  // Get the descriptor of a watchpoint based on its index.
  static object_metadata *access(uintptr_t index) throw();

  // Free the descriptor.
  static void free(object_metadata *, uintptr_t) throw();

  // Not used but needed.
  static void assign(object_metadata *, uintptr_t) throw() {}
};

namespace wp {

// Tell the watchpoints framework that
template <typename>
struct descriptor_type {
  typedef object_metadata type;
  enum {
    ALLOC_AND_INIT = true,
    REINIT_WATCHED_POINTERS = false
  };
};

}  // namespace wp
}  // namespace client

#endif  // CLIENTS_LIFETIME_METADATA_H_
