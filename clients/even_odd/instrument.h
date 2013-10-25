/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.h
 *
 *  Created on: Oct 25, 2013
 *      Author: pag
 */

#ifndef EVEN_ODD_INSTRUMENT_H_
#define EVEN_ODD_INSTRUMENT_H_


#include "granary/client.h"

#ifndef GRANARY_INIT_POLICY
#   define GRANARY_INIT_POLICY (client::even_policy())
#endif

namespace client {
    DECLARE_POLICY(even_policy, false);
    DECLARE_POLICY(odd_policy, false);
}



#endif /* EVEN_ODD_INSTRUMENT_H_ */
