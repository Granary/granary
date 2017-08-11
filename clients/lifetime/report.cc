/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "clients/lifetime/metadata.h"

namespace client {

extern object_metadata DESCRIPTORS[];

/// Report on all instrumented basic blocks.
void report(void) {
  // TODO(akshay): For each `data_structure_metadata`, dump all the statistics
  //               that we have about that data-structure using `granary::log`
  //               (it's like `printf`, but you might need to use `sprintf` for
  //               formatting. Take a look at `clients/cfg/report.cc` for an
  //               example.).

  // TODO(akshay): When reporting return addresses, we want to convert them
  //               from their cache code address back into native code
  //               addresses. Ask me about this later.

  UNUSED(DESCRIPTORS);
}

}  // namespace client
