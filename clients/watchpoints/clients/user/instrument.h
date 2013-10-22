/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-10-21
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_USER_POLICY_H_
#define WATCHPOINT_USER_POLICY_H_

#include "clients/watchpoints/instrument.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::watchpoint_user_policy())
#endif

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
namespace client {

    DECLARE_READ_WRITE_POLICY(
        user_policy /* name */,
        false /* auto-instrument */)

    DECLARE_INSTRUMENTATION_POLICY(
        watchpoint_user_policy,
        user_policy /* app read/write policy */,
        user_policy /* host read/write policy */,
        { /* override declarations */ })
}
#endif /* GRANARY_DONT_INCLUDE_CSTDLIB */
#endif /* WATCHPOINT_USER_POLICY_H_ */
