/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: 2013-10-26
 *      Author: Peter Goodman
 */

#ifndef SINGLE_EVEN_ODD_INSTRUMENT_H_
#define SINGLE_EVEN_ODD_INSTRUMENT_H_


#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::even_odd_policy())
#endif

namespace client {
    DECLARE_POLICY(even_odd_policy, false);
}


#endif /* SINGLE_EVEN_ODD_INSTRUMENT_H_ */
