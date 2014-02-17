/* Copyright 2014 Peter Goodman, all rights reserved. */

#include "clients/lifetime/metadata.h"

namespace client {

extern metadata DESCRIPTORS[];

/// Report on all instrumented basic blocks.
void report(void) throw() {
  UNUSED(DESCRIPTORS);
}

}  // namespace client
