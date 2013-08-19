/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include "granary/client.h"

namespace client {

    /// Invoked when an RCU read-side critical section extend beyond the
    /// function in which the read-side critical section was started. This seems
    /// like bad practice as it splits up the read-critical sections, and makes
    /// it more likely for bugs to creep in. This could also be evidence of a
    /// read-side critical section being left locked.
    granary::app_pc EVENT_READ_THROUGH_RET;


    /// A trailing call to a read-side critical section unlock point is invoked.
    /// This unlock does not match a lock.
    granary::app_pc EVENT_TRAILING_RCU_READ_UNLOCK;


    /// Invoked when there is an rcu_dereference outside of a read-side
    /// critical section.
    granary::app_pc EVENT_DEREF_OUTSIDE_OF_SECTION;


    /// Invoked when there is an rcu_assign_pointer within a read-side
    /// critical section.
    granary::app_pc EVENT_ASSIGN_IN_SECTION;
}


#endif /* EVENTS_H_ */
