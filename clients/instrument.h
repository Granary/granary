/*
 * instrument.h
 *
 *  Created on: 2013-04-24
 *      Author: pag
 */

#ifndef INSTRUMENT_H_
#define INSTRUMENT_H_

/// Null policy.
#ifdef CLIENT_NULL
#   include "clients/null_policy.h"
#endif


/// Null watchpoint policy.
#ifdef CLIENT_WATCHPOINT_NULL
#   include "clients/watchpoints/policies/null_policy.h"
#endif


/// Default to NULL policy if no policy was chosen.
#ifndef GRANARY_INIT_POLICY
#   include "clients/null_policy.h"
#endif


#endif /* INSTRUMENT_H_ */
