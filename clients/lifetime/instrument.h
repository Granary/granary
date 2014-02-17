/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef CLIENTS_LIFETIME_INSTRUMENT_H_
#define CLIENTS_LIFETIME_INSTRUMENT_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::lifetime())
#endif

namespace client {

DECLARE_READ_WRITE_POLICY(
    lifetime /* name */,
    false /* auto-instrument */)

DECLARE_INSTRUMENTATION_POLICY(
    lifetime,
    lifetime /* app read/write policy */,
    lifetime /* host read/write policy */,
    { /* override declarations */ })

}  // namespace client

#endif  // CLIENTS_LIFETIME_INSTRUMENT_H_
