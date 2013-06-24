/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_policy.h
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_BOUND_POLICY_H_
#define WATCHPOINT_BOUND_POLICY_H_

#include "clients/watchpoints/policies/leak_detector/policy_exit.h"
#include "clients/watchpoints/policies/leak_detector/policy_enter.h"
#include "clients/watchpoints/policies/leak_detector/policy_continue.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::leak_policy_enter())
#endif


#endif /* WATCHPOINT_BOUND_POLICY_H_ */
