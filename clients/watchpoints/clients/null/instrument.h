/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * null_policy.h
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#ifndef WATCHPOINT_NULL_POLICY_H_
#define WATCHPOINT_NULL_POLICY_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::watchpoint_null_policy())
#endif

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
namespace client {

    DECLARE_READ_WRITE_POLICY(
        null_policy /* name */,
        false /* auto-instrument */)

    DECLARE_INSTRUMENTATION_POLICY(
        watchpoint_null_policy,
        null_policy /* app read/write policy */,
        null_policy /* host read/write policy */,
        { /* override declarations */ })
}
#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
#endif /* WATCHPOINT_NULL_POLICY_H_ */
