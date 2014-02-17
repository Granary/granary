/* Copyright 2014 Peter Goodman, all rights reserved. */

#ifndef CLIENTS_LIFETIME_METADATA_H_
#define CLIENTS_LIFETIME_METADATA_H_

#include "clients/watchpoints/instrument.h"

namespace client {

// Meta-data about allocated objects.
struct metadata {
  static bool allocate_and_init(
      // Fixed arguments required by the watchpoints system.
      metadata *&desc,
      uintptr_t &counter_index,
      const uintptr_t inherited_index,

      // Arguments, you can add more, but make sure they match invocations
      // to `add_watchpoint`.
      void *addr_,
      size_t size_,
      void *allocator_addr_) throw();

  // Get the descriptor of a watchpoint based on its index.
  static metadata *access(uintptr_t index) throw();

  // Not used but needed.
  static void free(metadata *, uintptr_t) throw() {}
  static void assign(metadata *, uintptr_t) throw() {}

  // Fill me in!
  void *addr;
  unsigned size;
  void *allocator_addr;
};

namespace wp {

// Tell the watchpoints framework that
template <typename>
struct descriptor_type {
  typedef metadata type;
  enum {
    ALLOC_AND_INIT = true,
    REINIT_WATCHED_POINTERS = false
  };
};

}  // namespace wp
}  // namespace client

#endif  // CLIENTS_LIFETIME_METADATA_H_
