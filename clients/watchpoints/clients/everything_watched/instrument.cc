/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * watched_policy.cc
 *
 *  Created on: 2013-05-12
 *      Author: Peter Goodman
 */


#include "clients/watchpoints/clients/everything_watched/instrument.h"

using namespace granary;

namespace client {
    DEFINE_READ_VISITOR(watched_policy, {})
    DEFINE_WRITE_VISITOR(watched_policy, {})
    DEFINE_INTERRUPT_VISITOR(watched_policy, {})
} /* client namespace */


