/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * ark_policy.h
 *
 *  Created on: 2013-11-06
 *      Author: zymoticb
 */

#ifndef WATCHPOINT_ARK_POLICY_H_
#define WATCHPOINT_ARK_POLICY_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::watchpoint_ark_policy())
#endif

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
namespace client {

    DECLARE_READ_WRITE_POLICY(
       ark_policy /* name */,
        false /* auto-instrument */)

    DECLARE_INSTRUMENTATION_POLICY(
        watchpoint_ark_policy,
        ark_policy /* app read/write policy */,
        ark_policy /* host read/write policy */,
        { /* override declarations */ })
}
#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
#endif /* WATCHPOINT_ARK_POLICY_H_ */
