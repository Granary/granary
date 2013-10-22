/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * null_policy.cc
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#include "clients/watchpoints/clients/null/instrument.h"

using namespace granary;

namespace client {
    DEFINE_READ_VISITOR(null_policy, {})
    DEFINE_WRITE_VISITOR(null_policy, {})
    DEFINE_INTERRUPT_VISITOR(null_policy, {})
} /* client namespace */
