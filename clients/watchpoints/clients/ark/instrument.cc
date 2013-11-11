/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * ark_policy.cc
 *
 *  Created on: 2013-11-06
 *      Author: zymoticb
 */

#include "clients/watchpoints/clients/ark/instrument.h"

using namespace granary;

namespace client {
    DEFINE_READ_VISITOR(ark_policy, {})
    DEFINE_WRITE_VISITOR(ark_policy, {})
    DEFINE_INTERRUPT_VISITOR(ark_policy, {})
} /* client namespace */
