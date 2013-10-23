/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: Nov 20, 2012
 *      Author: pag
 */

#ifndef NULL_POLICY_H_
#define NULL_POLICY_H_

#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::null_policy())
#endif

namespace client {
    DECLARE_POLICY(null_policy, false);
}

#endif /* NULL_POLICY_H_ */
