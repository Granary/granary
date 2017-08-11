/* Copyright 2017 Peter Goodman (peter@trailofbits.com), all rights reserved. */

#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::tracer_policy())
#endif

namespace client {
DECLARE_POLICY(tracer_policy, true);
}  // namespace client
